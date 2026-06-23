# Agriculture auto robot project

## Hardware Platform
- **MCU**: STM32F407VET (168 MHz 32-bit Cortex-M4, 192 KB RAM, 512 KB)
- **Power**: Pin LiPo/Li-ion 24V-48V cho động cơ và hạ áp xuống 3.3V/5V cho MCU
- **Step driver**: CL57C 24V-48V, 200Khz, Minimal pulse width 2.5 us

## Real-Time Constraints
- a frame (formatted) will be sent from Master (PC or raspberry) through wifi, websever, bluetooth, UART-Serial
- cmd task receive the frame by interrupt, if a frame is detected it will be parsed to interface MotorMessage_t
- cmd task save the parsed interface MotorMessage_t to message queue
- motor task read queue and execute coresponding task base on MotorMessage_t

## Architecture
- **OS** (using zephyr)
- Currently, there are 2 tasks: cmd_task, motor_task
### Motor task
- motor task is designed following layer design.
- motor task included following component: motor_task, motor_app, motor_sm, motor_drv, abstraction layer, motor_hw
- motor_task: Coordinate between component and communicate with other task through interface (MotorMessage_t), interactive with RTOS (message queue)
#### motor_app
- application-specific support functions. These might include features like gathering telemetry, checking for faults
#### motor_sm 
- state machine which tracks the current state of the motor and the desired state. MOTOR_INIT -> MOTOR_READY -> MOTOR_RUNNING -> MOTOR_STOP -> MOTOR_FAULT.
**Role**: Manages motor transition logic, ensuring safe operational sequences and error recovery.
**Design Pattern**: Implemented as a **State Pattern** (using a State Transition Table). Each state is a class inheriting from a base `State` interface.
**State Definitions**:
- `MOTOR_INIT`: 
    - Performs hardware initialization.
    - **Critical Requirement**: Executes `Motor_HealthCheck()`. This includes verifying driver response (e.g., via a ping/status request) and checking supply voltage/safety status.
    - **Transition Rule**: 
        - If `Motor_HealthCheck()` returns `STATUS_OK` -> Transition to `MOTOR_READY`.
        - If `Motor_HealthCheck()` returns `STATUS_ERROR` -> Transition to `MOTOR_FAULT`.
- `MOTOR_READY`: Waiting for commands from `cmd_task`.
- `MOTOR_RUNNING`: Executing movement command; telemetry being monitored.
- `MOTOR_STOP`: Controlled deceleration or emergency stop trigger.
- `MOTOR_FAULT`: Error detected (e.g., UART timeout, driver alarm); requires manual reset or watchdog trigger.

#### motor_drv
- driver designed to control the motor. It has no hardware dependencies. Its only dependency is the abstraction layer. this must not include header file of motor_hw

#### Motor Abstraction layer 
- breaks the dependency between the motor driver and the hardware, only define .h class (pure virtual class)
#### motor_hw: 
    Role: Acts as the lowest abstraction layer, isolating control logic from hardware registers. It provides atomic functions to transmit commands to the CL57C driver.
    Design Constraints:

    Dependency Injection: Business logic components must never directly call Zephyr hardware drivers (e.g., device_get_binding). All device pointers must be injected via the constructor or the Init() method to maintain loose coupling.  

    Zero Polling: Do not use while() loops to wait for hardware flags. Utilize Zephyr’s asynchronous APIs or interrupt-driven callbacks for all communication.  

    Hardware Agnostic: motor_hw must remain strictly agnostic of state-machine logic. It executes physical commands without knowledge of the current state of the motor.
    motor_hw will send pulse to step driver CL57C through pin PA8 of stm32 which using PWM
    motor_hw will disable/enable driver through PE3 (PE3 HIGH = enable motor, PE3 LOW == disable motor)
**Alarm/Fault Feedback**
    The CL57C driver provides an `ALM` (Alarm) output signal to indicate system errors. The `motor_hw` component must configure an External Interrupt (ISR) to monitor this signal in real-time. Upon activation, the ISR shall immediately notify a event to motor_task. motor_task will handle and change motor_sm to `MOTOR_FAULT` state.

**Transition Logic**:
- All state transitions must be logged for debugging.
- Any transition to `MOTOR_FAULT` must immediately trigger a stop command to the `motor_hw` layer.
- The SM does not block execution; it operates on an event-driven basis via `MotorMessage_t`.

**MotorMessage_t**
    typedef struct {
        MotorID_t ID; // ID of driver motor (CL57C 24V-48V)
        MotorState_t State;
        MotorDirection_t Direction;
        MotorSpeed_t Speed;
    } MotorMessage_t;

**Frame Format**
    |Header|Motor ID|OP code|Data | CRC | END |
    Header: start frame 0xAA;
    Motor ID: ID of driver motor (CL57C 24V-48V)
    OP code: DIR (direction) or SPE(speed)
    Data: 2byte Speed (RPM not Hz)
    CRC: check sum
    END: 0x55

### Cmd task
- cmd task is designed following layer design
- cmd task included following component: cmd_task, cmd_parser, cmd_receiver, cmd_transfer, Abstraction layer, cmd_hw
#### cmd_task
Coordinate between component and communicate with other task through interface (MotorMessage_t), interactive with RTOS (message queue)
#### cmd_parser 
parse frame from master (through UART/ bluetooth/ wifi/ web server) to interface (MotorMessage_t) which using to communicate with other tasks
call function parse from speed from frame to Hz. PULSE/Rev will define by #define, default is 1600 following below fomular:
Hz = RPM x Revolution / 60 (Revolution default is 1600)
#### cmd_receiver
handling signal which receive from master (through UART/ bluetooth/ wifi/ web server) then gather them into a legal frame. Its only dependency is the abstraction layer. this must not include header file of cmd_hw
#### cmd_transfer
create a frame which smt32 want to send to master (PC/raspberry...), Its only dependency is the abstraction layer. this must not include header file of cmd_hw
#### Cmd Abstraction layer 
breaks the dependency between the motor driver and the hardware. only define .h class (pure virtual class)

#### cmd_hw
- Role: Manages the physical data stream between the Master device and the STM32 MCU via UART-RS232.
- Implementation:
    Utilizes Zephyr UART Async API or UART Interrupt-driven API to ensure real-time performance.
    Employs a Ring Buffer (Zephyr sys_ring_buf) to buffer incoming data from the UART interface before passing it to the cmd_receiver component.
    Public Functions: CmdHW_Init(), CmdHW_Transmit(uint8_t *data, size_t len).

## Build System & Toolchain
- **Compiler**: GCC ARM Embedded 10.3 (arm-none-eabi-gcc)
- **Build**: GNU Make
- **Optimization**: -Os for size, -O2 for time-critical functions only
- **Linker**: The project leverages Zephyr’s dynamic linker script generation. Custom memory regions are managed via Device Tree overlays to ensure compatibility and scalability across the target MCU family
- **Debugger**: ST-Link V2 with OpenOCD

## Memory & Linker Strategy
- **Framework**: Zephyr Build System manages linker scripts dynamically based on the selected board configuration.
- **Custom Memory**: Any specific data placement (e.g., calibration tables, persistent robot state) must utilize Zephyr's `linker_sections` macro to ensure alignment and safe access.
- **Optimization**: Post-build analysis of the generated `.map` file is mandatory to ensure the firmware footprint stays within the low-power consumption requirements.
- **Memory Map**: Configured via Device Tree (DTS) to maintain portability across different STM32 variants.

## Coding Standards
- **Language**: C++
- **Style**: 4-space indents, max 100 chars per line
- **Naming**: 
  - Functions: `ModuleName_FunctionName()` (e.g., `Motor_Controller()`)
  - Variables: camelCase for local, g_PascalCase for global
  - Macros: ALL_CAPS_WITH_UNDERSCORES
- **Documentation**: All public functions must have Doxygen-style comments

## Critical Don'ts
- **NEVER** use `malloc()`, `free()`, or any dynamic allocation after initialization
- **NEVER** use `printf()` or any stdio functions (code bloat + undefined behavior)
- **NEVER** use floating point in time-critical paths (no hardware FPU usage in ISRs)
- **NEVER** poll sensors - use interrupt-driven reads where 
- **NEVER** change DIR when SPEED > 0, if speed > 0 => decelerate speed (PUL) -> 0, waiting 50ms - 100 ms, change logic of DIR (0 forward or 1 backward) => **waiting minimum 5us** => Accelerate speed (accelerate PUL)

## Allowed Libraries
- Using standard library of zephyr
- No external libraries without approval

## File Structure
my_agri_robot/
├── CMakeLists.txt              # Build system chính
├── prj.conf                    # Cấu hình tính năng Zephyr (Kconfig)
├── boards/                     # Overlay DeviceTree (.overlay)
├── src/
│   ├── main.c                  # Entry point (chỉ khởi tạo, không xử lý logic)
│   ├── app_tasks/              # Logic của các task (sử dụng k_thread)
│   │   ├── motor_task.c
│   │   └── cmd_task.c
│   ├── subsystems/             # Tận dụng các Subsystem của Zephyr
│   │   ├── motor_control/      # Driver logic tùy chỉnh (Wrap các API của Zephyr)
│   │   └── comms/              # Tận dụng UART_ASYNC của Zephyr
│   └── include/
│       └── app_config.h        # Define hằng số project
└── app.overlay                 # Định nghĩa các pin/peripheral trong DT

This structure can be changed to suitable with this project and zephyr folder.

## Error Handling Strategy

## Wiring
STM32F407VET (GPIO)                 Driver CL57 (P1 Connector)
      +-------------------+               +--------------------------+
      |                   |               |                          |
      |   (Nguồn 5V)------+-------------->| PUL+                     |
      |                   |               |                          |
      |   Pin [PA8] ------+-------------->| PUL-                     |
      |                   |               |                          |
      |                   |               |                          |
      |   (Nguồn 5V)------+-------------->| DIR+                     |
      |                   |               |                          |
      |   Pin [PE2] ------+-------------->| DIR-                     |
      |                   |               |                          |
      |                   |               |                          |
      |   (Nguồn 5V)------+-------------->| ENA+                     |
      |                   |               |                          |
      |   Pin [PE3] ------+-------------->| ENA-                     |
      |                   |               |                          |
      |                   |               |                          |
      |   GND  -----------+---------------+-- ALM-                   |
      |   Pin [PE4] <-----+---------------+-- ALM+                   |
      |                   |               |                          |
      +-------------------+               +--------------------------+
               ^
               |
        (Kết nối chung GND)


