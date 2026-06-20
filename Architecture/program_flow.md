# Program Flow & Build Guide - Agriculture Robot

## 1. Tổng quan luồng hoạt động

```
┌─────────────┐   UART/Serial   ┌──────────────────────────────────────────────┐
│  Master PC  │ ─────────────>  │                  STM32F407VET                │
│ (Raspberry) │ <─────────────  │                                              │
└─────────────┘   ACK/Status    │  ┌──────────┐        ┌──────────────────┐   │
                                │  │ cmd_task │ ──msg──>│   motor_task     │   │
                                │  │          │ queue   │                  │   │
                                │  │ cmd_hw   │         │  motor_sm (STT)  │   │
                                │  │ (UART)   │         │  motor_drv       │   │
                                │  │ cmd_recv │         │  motor_hw        │   │
                                │  │ cmd_parse│         │  (PWM/GPIO/ISR)  │   │
                                │  └──────────┘        └──────────────────┘   │
                                └──────────────────────────────────────────────┘
                                                              │
                                                              ▼
                                                  ┌───────────────────┐
                                                  │  CL57C Driver     │
                                                  │  PUL-: PA8 (PWM)  │
                                                  │  DIR-: PE2        │
                                                  │  ENA-: PE3        │
                                                  │  ALM+: PE4 (ISR)  │
                                                  └───────────────────┘
```

---

## 2. Startup Flow (Khởi động)

```
main()
 │
 ├─► MotorTask::Init()
 │     │
 │     ├─► MotorDrv::Init()
 │     │     └─► MotorHW::Init()
 │     │           ├─► PWM TIM1/CH1 (PA8) ready
 │     │           ├─► GPIO PE2 (DIR)  = OUTPUT LOW (FORWARD)
 │     │           ├─► GPIO PE3 (ENA)  = OUTPUT LOW (disabled)
 │     │           └─► GPIO PE4 (ALM)  = INPUT + ISR rising edge
 │     │
 │     ├─► MotorSM::Init()   → state = MOTOR_INIT
 │     │
 │     └─► MotorHW::HealthCheck()
 │           ├─ PE4 (ALM) == LOW?  → OK
 │           └─ GPIO device ready? → OK
 │                 │
 │           HealthCheck OK  → SM.ProcessEvent(EVT_INIT_OK)  → state = MOTOR_READY
 │           HealthCheck FAIL→ SM.ProcessEvent(EVT_INIT_FAIL) → state = MOTOR_FAULT
 │
 └─► CmdTask::Init()
       ├─► CmdHW::Init()
       │     ├─► USART2 init (115200 baud, PA2/PA3)
       │     ├─► sys_ring_buf init (256 bytes)
       │     └─► uart_rx_enable() (UART async API)
       │
       └─► CmdReceiver::Init()
             └─► Đăng ký byte callback với CmdHW

K_THREAD_DEFINE: motor_task thread (priority 5)
K_THREAD_DEFINE: cmd_task thread   (priority 3)  ← ưu tiên cao hơn
```

---

## 3. Frame Protocol (Giao thức khung dữ liệu)

### 3.1 Cấu trúc frame (7 bytes)

```
Byte 0  │ Byte 1   │ Byte 2  │ Byte 3  │ Byte 4  │ Byte 5 │ Byte 6
─────────┼──────────┼─────────┼─────────┼─────────┼────────┼───────
 0xAA   │ MotorID  │ Opcode  │ DataHI  │ DataLO  │  CRC   │  0x55
Header  │ (1 byte) │ (1 byte)│ (1 byte)│ (1 byte)│ (XOR)  │  End
```

**CRC**: `XOR(MotorID, Opcode, DataHI, DataLO)`

### 3.2 Bảng Opcode

| Opcode | Hex  | Chức năng       | DataHI       | DataLO        |
|--------|------|-----------------|--------------|---------------|
| SPE    | 0x01 | Set tốc độ (Hz) | Speed >> 8   | Speed & 0xFF  |
| DIR    | 0x02 | Set chiều quay  | 0=FWD/1=BWD  | 0x00          |
| STOP   | 0x03 | Dừng motor      | 0x00         | 0x00          |
| RESET  | 0x04 | Reset lỗi       | 0x00         | 0x00          |

---

## 4. Ví dụ thực tế: Quay 600 RPM

### 4.1 Tính tần số pulse (Hz)

```
Công thức:
  Pulse_Hz = (RPM × Pulses_Per_Revolution) / 60

Cấu hình CL57C (điều chỉnh qua DIP switch SW3/SW4):
  Phổ biến nhất: 1600 pulses/rev (microstep x8 với motor 1.8°)
  
Ví dụ với 1600 pulses/rev:
  Pulse_Hz = (600 × 1600) / 60 = 16000 Hz = 0x3E80
```

**Bảng quy đổi RPM → Hz theo cấu hình CL57C:**

| Pulses/Rev | 100 RPM | 300 RPM  | 600 RPM  | 1200 RPM |
|------------|---------|----------|----------|----------|
| 200        | 333 Hz  | 1000 Hz  | 2000 Hz  | 4000 Hz  |
| 400        | 667 Hz  | 2000 Hz  | 4000 Hz  | 8000 Hz  |
| 800        | 1333 Hz | 4000 Hz  | 8000 Hz  | 16000 Hz |
| **1600**   |2667 Hz  | 8000 Hz  |**16000 Hz**|32000 Hz|
| 3200       | 5333 Hz | 16000 Hz | 32000 Hz | 64000 Hz |

### 4.2 Chuỗi lệnh UART để quay 600 RPM (chiều thuận)

**Bước 1: Set chiều quay FORWARD**
```
Byte:  AA   01   02   00   00   03   55
       HDR  ID   DIR  FWD  ---  CRC  END

CRC = 0x01 XOR 0x02 XOR 0x00 XOR 0x00 = 0x03
```

**Bước 2: Set tốc độ 16000 Hz (600 RPM @ 1600 p/r)**
```
Byte:  AA   01   01   3E   80   BE   55
       HDR  ID   SPE  HI   LO   CRC  END

Speed = 16000 = 0x3E80
  DataHI = 0x3E
  DataLO = 0x80
  CRC = 0x01 XOR 0x01 XOR 0x3E XOR 0x80 = 0xBE
```

**Bước 3 (khi muốn dừng): STOP**
```
Byte:  AA   01   03   00   00   02   55
       HDR  ID   STO  ---  ---  CRC  END

CRC = 0x01 XOR 0x03 XOR 0x00 XOR 0x00 = 0x02
```

**Bước 4 (nếu có lỗi): RESET**
```
Byte:  AA   01   04   00   00   05   55
       HDR  ID   RST  ---  ---  CRC  END

CRC = 0x01 XOR 0x04 XOR 0x00 XOR 0x00 = 0x05
```

### 4.3 Python script gửi lệnh (test trên PC)

```python
import serial
import time

def calc_crc(motor_id, opcode, data_hi, data_lo):
    return motor_id ^ opcode ^ data_hi ^ data_lo

def build_frame(motor_id, opcode, data_hi=0, data_lo=0):
    crc = calc_crc(motor_id, opcode, data_hi, data_lo)
    return bytes([0xAA, motor_id, opcode, data_hi, data_lo, crc, 0x55])

def rpm_to_hz(rpm, pulses_per_rev=1600):
    return int((rpm * pulses_per_rev) / 60)

ser = serial.Serial('COM3', 115200, timeout=1)

MOTOR_ID = 0x01
TARGET_RPM = 600
speed_hz = rpm_to_hz(TARGET_RPM)   # = 16000 Hz

# 1. Set direction FORWARD
frame_dir = build_frame(MOTOR_ID, 0x02, 0x00, 0x00)
ser.write(frame_dir)
print(f"DIR FWD: {frame_dir.hex(' ').upper()}")
time.sleep(0.1)

# 2. Set speed 600 RPM
data_hi = (speed_hz >> 8) & 0xFF
data_lo = speed_hz & 0xFF
frame_spd = build_frame(MOTOR_ID, 0x01, data_hi, data_lo)
ser.write(frame_spd)
print(f"SPE {TARGET_RPM}RPM ({speed_hz}Hz): {frame_spd.hex(' ').upper()}")
time.sleep(5)   # chạy 5 giây

# 3. Stop
frame_stop = build_frame(MOTOR_ID, 0x03, 0x00, 0x00)
ser.write(frame_stop)
print(f"STOP: {frame_stop.hex(' ').upper()}")

ser.close()
```

---

## 5. Luồng xử lý khi gửi lệnh SPE

```
Master (PC)
  │ gửi: AA 01 01 3E 80 BE 55 (16000 Hz = 600 RPM)
  ▼
USART2 RX (PA3) - hardware interrupt
  │
  ▼
CmdHW::UartAsyncCallback() [ISR context]
  ├─► ring_buf_put() ← lưu bytes vào ring buffer (256B)
  └─► DrainRingBuffer() ← gọi OnByteReceived() cho từng byte
          │
          ▼
CmdReceiver::ProcessByte() [ISR context]
  ├─ byte=0xAA → state=RECV_BODY, frameIdx=1
  ├─ byte=0x01 → frameIdx=2
  ├─ byte=0x01 → frameIdx=3
  ├─ byte=0x3E → frameIdx=4
  ├─ byte=0x80 → frameIdx=5
  ├─ byte=0xBE → frameIdx=6
  └─ byte=0x55 → frameIdx=7 == FRAME_SIZE
        ├─ Check END == 0x55 ✓
        ├─ Check CRC: 0x01^0x01^0x3E^0x80 = 0xBE ✓
        └─► FrameCallback(frame) → k_msgq_put(frameQueue)  [ISR-safe]

cmd_task thread (priority 3) - unblocked từ k_msgq_get()
  │
  ▼
CmdTask::HandleFrame()
  ├─► CmdParser::Parse()
  │     └─► msg = { ID=1, State=RUNNING, Speed=16000, Dir=FORWARD }
  │
  ├─► MotorTask::PostMessage(msg) → k_msgq_put(motorQueue)
  │
  └─► CmdTransfer::SendAck(motorID=1, opcode=0x01)
        └─► uart_tx(): AA 01 81 01 00 81 55  (ACK frame)

motor_task thread (priority 5) - unblocked từ k_msgq_get()
  │
  ▼
MotorTask::DispatchMessage()
  ├─► MotorApp::NotifyRunStart()  ← ghi start time
  └─► MotorSM::ProcessEvent(EVT_CMD_RUN, &msg)
        │
        └─► [state=READY] HandleReady()
              ├─► MotorDrv::Run(FORWARD, 16000)
              │     ├─ dir == currentDir (FORWARD) → không cần delay
              │     ├─► MotorHW::Enable(true)  → PE3 = HIGH
              │     └─► AccelerateToTarget(16000)
              │           ├─ speed=200  → PWM period=5000us, pulse=5us
              │           ├─ k_msleep(10ms)
              │           ├─ speed=400  → PWM ...
              │           ├─ ...
              │           └─ speed=16000 → PWM period=62.5us, pulse=5us
              │                               ↑ 600 RPM đạt được
              │
              └─► TransitionTo(MOTOR_RUNNING) ← log "READY -> RUNNING"
```

---

## 6. Luồng đổi chiều (Direction Change Safety Sequence)

```
Trạng thái: MOTOR_RUNNING tại 16000 Hz (FORWARD)

Master gửi: DIR BACKWARD + SPE 16000
  ↓
MotorDrv::Run(BACKWARD, 16000)
  │
  ├─ PHÁT HIỆN: targetDir(BWD) != currentDir(FWD) && currentSpeed > 0
  │             → SAFETY SEQUENCE bắt đầu
  │
  ├─► DecelerateToZero()
  │     ├─ speed: 16000 → 15800 → ... → 200 → 0
  │     ├─ k_msleep(10ms) mỗi bước
  │     └─ MotorHW::StopPulse()  ← pulse_width = 0
  │
  ├─► k_msleep(75ms)   ← Chờ 50-100ms (architecture requirement)
  │
  ├─► MotorHW::SetDirection(BACKWARD)  ← PE2 = HIGH
  │
  ├─► k_busy_wait(5us)  ← Tối thiểu 5us setup time
  │
  └─► AccelerateToTarget(16000)
        └─ speed: 200 → 400 → ... → 16000 Hz (BACKWARD)
```

---

## 7. Luồng ALM (Alarm/Fault)

```
CL57C phát hiện lỗi (quá dòng, quá nhiệt, v.v.)
  │
  ▼
ALM+ pin (PE4) → HIGH (rising edge)
  │
  ▼
GPIO ISR: MotorHW::AlmIsrHandler() [ISR context]
  └─► k_event_post(&events, EVT_ALM)  ← wakeup motor_task

motor_task (sleeping tại k_msgq_get timeout 100ms)
  ├─► k_event_wait(EVT_ALM, K_NO_WAIT) → detected
  ├─► MotorApp::NotifyAlarm()  ← faultCount++
  └─► MotorSM::ProcessEvent(MOTOR_EVT_FAULT)
        └─► TransitionTo(MOTOR_FAULT)
              ├─► MotorDrv::EmergencyStop()
              │     ├─ MotorHW::StopPulse()   ← PWM = 0 ngay lập tức
              │     └─ MotorHW::Enable(false) ← PE3 = LOW
              └─► LOG "RUNNING -> FAULT"

Để recovery: Master gửi RESET frame
  AA 01 04 00 00 05 55
  → MotorSM::EVT_RESET → MOTOR_INIT → HealthCheck → MOTOR_READY
```

---

## 8. State Machine Diagram

```
                    ┌─────────────┐
              ┌────►│ MOTOR_INIT  │◄────────────────────────────────┐
              │     └──────┬──────┘                                  │
         EVT_RESET         │                                      EVT_RESET
              │     HealthCheck OK                                   │
              │            │ EVT_INIT_OK          HealthCheck FAIL   │
              │            ▼                            │            │
              │     ┌─────────────┐               ┌────▼────────┐   │
              │     │ MOTOR_READY │               │ MOTOR_FAULT │───┘
              │     └──────┬──────┘               └─────────────┘
              │     EVT_CMD_RUN │                       ▲
              │            │                            │ EVT_FAULT (any state)
              │            ▼                            │
              │     ┌──────────────┐  EVT_CMD_STOP  ┌──┴──────────┐
              │     │ MOTOR_RUNNING├───────────────►│ MOTOR_STOP  │
              │     └──────────────┘                └──────┬───────┘
              │                                    EVT_STOPPED │
              └────────────────────────────────────────────────┘
```

---

## 9. Hướng dẫn Build

### 9.1 Yêu cầu cài đặt

**Bước 1: Cài Python & west**
```powershell
# Python 3.10+ (đã có)
pip install west
```

**Bước 2: Cài Zephyr SDK (toolchain ARM)**
```powershell
# Download Zephyr SDK 0.16.x từ:
# https://github.com/zephyrproject-rtos/sdk-ng/releases
# File: zephyr-sdk-0.16.8_windows-x86_64.exe
# Cài vào: C:\zephyr-sdk-0.16.8

# Sau khi cài, set biến môi trường:
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.16.8"
```

**Bước 3: Cài Python dependencies của Zephyr**
```powershell
cd D:\agriculture\Robot_Farm\zephyrproject\zephyr
pip install -r scripts/requirements.txt
```

### 9.2 Set biến môi trường

```powershell
# Tạm thời (terminal hiện tại)
$env:ZEPHYR_BASE = "D:\agriculture\Robot_Farm\zephyrproject\zephyr"

# Hoặc thêm vào profile PowerShell ($PROFILE) để persistent:
# $env:ZEPHYR_BASE = "D:\agriculture\Robot_Farm\zephyrproject\zephyr"
```

### 9.3 Build

```powershell
# Vào thư mục zephyrproject
cd D:\agriculture\Robot_Farm\zephyrproject

# Build cho STM32F4 Discovery (STM32F407VGT - gần nhất với VET)
west build -b stm32f4_disco ./my_agri_robot

# Build sạch (nếu thay đổi overlay/Kconfig)
west build -b stm32f4_disco ./my_agri_robot --pristine

# Build cho board custom (nếu có board definition riêng cho VET)
west build -b my_stm32f407vet ./my_agri_robot
```

**Output sau khi build thành công:**
```
-- Build files have been written to: .../my_agri_robot/build
[200/200] Linking CXX executable zephyr/zephyr.elf
Memory region     Used Size  Region Size  %age Used
           FLASH:     xxxxx B       512 KB    xx.xx%
            SRAM:     xxxxx B       192 KB    xx.xx%
```

### 9.4 Flash lên STM32

```powershell
# Flash qua ST-Link V2 (OpenOCD)
west flash

# Hoặc chỉ định runner
west flash --runner openocd

# Flash file .bin thủ công với STM32CubeProgrammer
# File nằm tại: ./my_agri_robot/build/zephyr/zephyr.bin
```

### 9.5 Debug

```powershell
# Mở debug session (GDB + OpenOCD)
west debug

# Xem log qua RTT hoặc UART
# UART log: kết nối PA2 (TX) vào USB-Serial, mở terminal 115200 baud
```

### 9.6 Cấu trúc thư mục build output

```
my_agri_robot/build/
├── zephyr/
│   ├── zephyr.elf    ← debug với GDB
│   ├── zephyr.bin    ← flash thủ công
│   ├── zephyr.hex    ← flash qua STM32CubeProgrammer
│   └── zephyr.map    ← phân tích memory footprint
└── CMakeFiles/
```

---

## 10. Bảng tính nhanh RPM → Frame UART

Với **1600 pulses/rev** (CL57C DIP: microstep x8 @ 200 step/rev motor):

| RPM   | Hz     | DataHI | DataLO | Frame (Motor ID=0x01, FORWARD) |
|-------|--------|--------|--------|-------------------------------|
| 60    | 1600   | 0x06   | 0x40   | `AA 01 01 06 40 44 55`        |
| 120   | 3200   | 0x0C   | 0x80   | `AA 01 01 0C 80 8E 55`        |
| 300   | 8000   | 0x1F   | 0x40   | `AA 01 01 1F 40 5F 55`        |
| **600** | **16000** | **0x3E** | **0x80** | **`AA 01 01 3E 80 BE 55`** |
| 900   | 24000  | 0x5D   | 0xC0   | `AA 01 01 5D C0 1D 55`        |
| 1200  | 32000  | 0x7D   | 0x00   | `AA 01 01 7D 00 7D 55`        |

> **Lưu ý**: Nếu CL57C cấu hình khác (ví dụ 800 pulses/rev), nhân đôi tần số ở cột Hz.

---

## 11. Troubleshooting Build

| Lỗi | Nguyên nhân | Giải pháp |
|-----|-------------|-----------|
| `ZEPHYR_BASE not set` | Chưa set biến môi trường | `$env:ZEPHYR_BASE = "...zephyr"` |
| `arm-none-eabi-gcc not found` | Chưa cài Zephyr SDK | Cài SDK và set `ZEPHYR_SDK_INSTALL_DIR` |
| `DT_NODELABEL(pwm1) not found` | Overlay sai | Kiểm tra `app.overlay`, tên node phải khớp board |
| `uart_callback_set failed` | UART không hỗ trợ async | Thêm `CONFIG_UART_ASYNC_API=y` vào `prj.conf` |
| `undefined reference to MotorHW` | CMakeLists thiếu source | Kiểm tra `target_sources()` trong CMakeLists.txt |
