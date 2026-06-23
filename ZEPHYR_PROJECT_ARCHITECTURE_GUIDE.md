# Hướng dẫn tổ chức project Zephyr cho RoboLife Farm

Tài liệu này gợi ý cách đặt project firmware sử dụng Zephyr trong repo `RoboLife_Farm`, đồng thời đề xuất architecture thư mục phù hợp cho project nhúng phát triển dài hạn.

## 1. Nguyên tắc đặt project Zephyr

Không nên đặt application firmware trực tiếp bên trong thư mục source của Zephyr:

```text
D:\DOCUMENT\RoboLife_Farm\zephyrproject\zephyr\my_app
```

Lý do:

- `zephyrproject/zephyr/` là source framework/RTOS của Zephyr.
- Dễ bị rối khi update Zephyr, checkout branch, rebase hoặc đồng bộ workspace.
- Khó quản lý Git nếu code ứng dụng bị trộn với source Zephyr.
- Khó tách riêng firmware application, tài liệu, test và script build.

Nên xem:

```text
zephyrproject/zephyr/
```

là SDK/framework, còn firmware application của dự án nên nằm ở một thư mục riêng bên ngoài.

## 2. Cấu trúc thực tế của repo

```text
AgricultureProject/
├── Architecture/               # Tài liệu thiết kế, bug analysis
├── Tools/
├── Firmware/                   # Firmware application (tách khỏi Zephyr workspace)
│   └── my_agri_robot/          # App Zephyr cho STM32F407
│       ├── CMakeLists.txt
│       ├── prj.conf
│       ├── app.overlay
│       └── src/
│           ├── main.cpp
│           ├── app_tasks/          # cmd_task, motor_task
│           ├── subsystems/
│           │   ├── comms/          # cmd_hw, cmd_receiver, cmd_parser, cmd_transfer
│           │   └── motor_control/  # motor_hw, motor_drv, motor_sm, motor_app
│           └── include/            # app_config.h, motor_interface.h
└── zephyrproject/              # Zephyr RTOS workspace (không chứa app)
    ├── zephyr/
    ├── modules/
    ├── bootloader/
    └── tools/
```

Trong đó:

- `zephyrproject/`: workspace Zephyr thuần tuý — không đặt application vào đây.
- `Firmware/`: chứa toàn bộ source firmware application.
- `my_agri_robot/`: app Zephyr chạy trên STM32F407VET với CL57C step driver.

## 3. Build command

Vì app nằm ngoài `zephyrproject/`, cần set `ZEPHYR_BASE` trước khi build:

```bash
# Thiết lập môi trường Zephyr (chạy một lần mỗi terminal)
source ~/AgricultureProject/zephyrproject/zephyr/zephyr-env.sh

# Build app từ thư mục Firmware/my_agri_robot
west build -b black_f407ve Firmware/my_agri_robot
```

Hoặc chỉ định thư mục app trực tiếp:

```bash
west build -b black_f407ve -- -DAPP_DIR=Firmware/my_agri_robot
```

`CMakeLists.txt` dùng `find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})` nên tương thích với cả hai cách.

## 4. Nguyên tắc tổng quát

- `zephyrproject/` dùng cho Zephyr workspace — chỉ chứa RTOS và modules.
- `Firmware/` dùng cho firmware application — một subdirectory cho mỗi board/target.
- Không viết app trực tiếp vào `zephyrproject/`.
- Nên tách rõ `app_tasks`, `subsystems`, `include` để dễ mở rộng và test.