# Farm Robot Telegram Control

Tool Python điều khiển và giám sát robot/laptop từ xa qua Telegram Bot. Toàn bộ tool nằm trong thư mục `telegram_control/` để không phá project robot chính.

## AI DevOps qua Telegram

Hệ thống hỗ trợ gửi yêu cầu lập trình từ Telegram:

```text
/code Thêm API đọc encoder MKS Servo42C
```

Luồng xử lý:

1. Bot lưu task JSON vào `telegram_control/tasks/pending/`.
2. AI DevOps worker đọc task.
3. Worker gọi OpenAI API bằng `OPENAI_MODEL` để đọc context source code và sinh plan + danh sách file thay đổi.
4. Bot gửi plan lên Telegram.
5. Người dùng duyệt bằng `/approve TASK_xxx` hoặc từ chối bằng `/reject TASK_xxx lý do`.
6. Chỉ sau khi approve, hệ thống mới ghi file source code.
7. Hệ thống build bằng `BUILD_COMMAND` nếu được cấu hình.
8. Nếu build OK, hệ thống commit theo format `[AI TASK] <task title>`.
9. Hệ thống chỉ push lên branch `devQuang`, không merge/push `main`.
10. Bot gửi kết quả/log/diff về Telegram.

### Cấu hình AI DevOps

Trong `telegram_control/.env`:

```env
AI_DEVOPS_ENABLED=1
AI_TASK_POLL_SECONDS=5
OPENAI_API_KEY=sk-...
OPENAI_MODEL=gpt-5.5
AUTO_PUSH_BRANCH=devQuang
MAX_SOURCE_CHARS=120000
```

> Lưu ý: `gpt-5.5` phải là model hợp lệ với API key của bạn. Nếu OpenAI chưa hỗ trợ tên model này trong tài khoản, đổi sang model bạn có quyền dùng.

### Lệnh AI DevOps

```text
/code <nội dung>        Tạo AI coding task
/tasks                  Xem task queue
/approve <task_id>      Duyệt plan và cho phép sửa code/build/commit/push
/reject <task_id>       Từ chối task
/log <task_id>          Xem log task
/diff <task_id>         Xem diff task
/build_now              Build ngay
/git-status             Xem git status
/git-push               Push branch devQuang
```

### Chạy hệ thống

Windows:

```bat
run_telegram_ai_devops.bat
```

Hoặc:

```bash
python -m telegram_control.main
```

## Cấu trúc hệ thống

```text
telegram_control/
├── main.py                 # Khởi tạo bot và đăng ký command
├── config.py               # Đọc cấu hình từ .env
├── commands/               # Mỗi Telegram command là một file riêng
├── services/               # Camera, build, flash, deploy, task, approval...
├── utils/                  # Auth, logger, file utils, command runner
├── tasks/                  # File-based workflow cho Cline/task
├── approvals/              # Approval request pending/approved/rejected
└── data/                   # Video, ảnh, log runtime
```

## Cài thư viện

```bash
cd telegram_control
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

## Tạo Telegram Bot bằng BotFather

1. Mở Telegram, tìm `@BotFather`.
2. Gửi `/newbot`.
3. Đặt tên bot và username bot.
4. Copy token vào `BOT_TOKEN` trong `.env`.

## Lấy chat_id, group_id, user_id

Nhắn tin cho bot hoặc thêm bot vào group, sau đó mở:

```text
https://api.telegram.org/bot<TOKEN>/getUpdates
```

Tìm `message.chat.id` cho chat/group và `message.from.id` cho user id. Group id thường là số âm.

## Cấu hình `.env`

```bash
copy .env.example .env
```

Ví dụ:

```env
BOT_TOKEN=token_bot_that
ALLOWED_CHAT_ID=123456789
ALLOWED_GROUP_ID=-1001234567890
ADMIN_USER_IDS=111111111
OPERATOR_USER_IDS=222222222
VIEWER_USER_IDS=333333333
PROJECT_ROOT=..
BUILD_COMMAND=pio run
FLASH_COMMAND=pio run -t upload
SERIAL_PORT=COM3
SERIAL_BAUDRATE=115200
```

## Phân quyền

- `ADMIN`: tạo task, approve/reject, build, flash, deploy, log, status, result.
- `OPERATOR`: tạo task, xem status/log, yêu cầu build/flash/deploy nhưng phải chờ ADMIN approve.
- `VIEWER`: xem status, log, result, ask_result.
- Người ngoài danh sách hoặc group sai `ALLOWED_GROUP_ID`: bot từ chối.

## Chạy bot

Chạy từ thư mục cha của `telegram_control`:

```bash
python -m telegram_control.main
```

## Lệnh chính

```text
/start
/help
/cam10
/photo
/status
/gitstatus
/task sửa thuật toán đọc encoder MKS Servo42C theo protocol mới
/build
/flash
/deploy
/result
/logs
/ask_result tóm tắt lỗi build và gợi ý cách sửa
/approve APPROVAL_xxx
/reject APPROVAL_xxx lý do
```

## Workflow approval

Các lệnh shell/hardware/code-sensitive không chạy ngay. Bot tạo approval file trong `approvals/pending/` và gửi về Telegram. ADMIN duyệt bằng:

```text
/approve APPROVAL_xxx
```

Hoặc từ chối:

```text
/reject APPROVAL_xxx lý do
```

Khi approved, bot chuyển file sang `approvals/approved/` rồi thực thi action trong process bot. Khi rejected, file chuyển sang `approvals/rejected/` và không thực thi.

## `/task` và Cline file-based workflow

`/task <nội dung>` tạo file JSON trong `tasks/pending/` gồm task id, thời gian, người tạo, chat id, nội dung và trạng thái.

Cline có thể đọc `tasks/pending/TASK_xxx.json`, xử lý, và ghi kết quả:

- Thành công: `tasks/done/TASK_xxx_result.md`
- Lỗi: `tasks/failed/TASK_xxx_error.md`

Bot dùng `/result` hoặc `/ask_result ...` để đọc/tổng hợp dữ liệu có thật từ log/result file. Nếu thiếu dữ liệu, bot báo rõ thiếu dữ liệu.

## Build/Flash/Deploy hardware

`BUILD_COMMAND` và `FLASH_COMMAND` chỉ lấy từ `.env`, không nhận raw shell input từ Telegram.

`/deploy` sau khi ADMIN approve sẽ build, dừng nếu lỗi, flash nếu build OK, đọc serial nếu flash OK, rồi gửi tổng hợp.

## Thêm command mới

1. Tạo `commands/battery_cmd.py`.
2. Viết handler async và dùng `@require_roles(...)`.
3. Nếu cần logic riêng, tạo `services/battery_service.py`.
4. Đăng ký trong `commands/__init__.py` bằng `CommandHandler`.

## Bảo mật

- Không commit `.env`.
- Không lưu token trong source code.
- Không cho nhập raw shell command từ Telegram.
- Chỉ chạy command cố định từ config.
- Build/flash/deploy/git status đều cần ADMIN approve.
- Không tạo lệnh xóa file, format ổ đĩa, tắt máy, reset máy.
- Video, ảnh, log, task runtime, approval runtime nằm trong `.gitignore`.

## Git commit

Theo yêu cầu hiện tại, tool không tự chạy git commit. Nếu muốn commit sau này, hãy xác nhận riêng rồi chạy:

```bash
git add telegram_control
git commit -m "Add Telegram remote control system"
```
