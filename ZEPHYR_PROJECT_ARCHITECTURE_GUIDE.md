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

## 2. Vị trí đề xuất trong repo hiện tại

Repo hiện tại:

```text
D:\DOCUMENT\RoboLife_Farm
├── Architecture/
├── Tools/
├── zephyrproject/
│   ├── zephyr/
│   ├── modules/
│   ├── bootloader/
│   └── tools/
```

Vị trí nên đặt firmware application:

```text
D:\DOCUMENT\RoboLife_Farm\Firmware\Farm_Controller_STM32F407
```

Cấu trúc tổng quan nên là:

```text
D:\DOCUMENT\RoboLife_Farm
├── Architecture/
├── Tools/
├── Firmware/
│   └── Farm_Controller_STM32F407/
└── zephyrproject/
    ├── zephyr/
    ├── modules/
    ├── bootloader/
    └── tools/
```

Trong đó:

- `zephyrproject/`: workspace Zephyr, chứa Zephyr RTOS và module liên quan.
- `Firmware/`: chứa source code firmware application của RoboLife Farm.
- `Farm_Controller_STM32F407/`: app Zephyr chạy trên board `black_f407ve` hoặc STM32F407 tương ứng.


Tóm lại:

- `zephyrproject/` dùng cho Zephyr workspace.
- `Firmware/` dùng cho firmware application của RoboLife Farm.
- Không nên viết app trực tiếp vào `zephyrproject/zephyr/`.
- Nên tách rõ `domain`, `services`, `drivers`, `protocol`, `utils` để project dễ mở rộng và dễ test.