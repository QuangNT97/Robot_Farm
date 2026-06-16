from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.utils.auth import ALL_AUTHORIZED, require_roles


HELP_TEXT = """Các lệnh hiện có:
/start - Kiểm tra bot sẵn sàng
/help - Danh sách lệnh
/cam10 - Quay video camera laptop 10 giây
/photo - Chụp ảnh camera laptop
/status - Trạng thái laptop/bot
/gitstatus - Tạo yêu cầu ADMIN duyệt để chạy git status
/code <nội dung> - Tạo AI DevOps task, sinh plan rồi chờ approve
/tasks - Xem hàng đợi AI task
/task <nội dung> - Tạo task file cho Cline xử lý
/build - Tạo approval build firmware
/build_now - Build ngay và gửi log
/flash - Tạo approval flash firmware
/deploy - Tạo approval build + flash + đọc serial
/result - Xem kết quả task gần nhất
/logs - Xem log hệ thống gần nhất
/log <task_id> - Xem log AI task
/diff <task_id> - Xem diff AI task
/ask_result <yêu cầu> - Tổng hợp kết quả từ log/result hiện có
/git-status hoặc /git_status - Git status ngay
/git-push hoặc /git_push - Push branch devQuang ngay
/run_project - Chạy project bằng RUN_PROJECT_COMMAND trong .env
/stop_project - Dừng project đang chạy bởi /run_project
/approve APPROVAL_xxx - ADMIN duyệt yêu cầu
/reject APPROVAL_xxx lý do - ADMIN từ chối yêu cầu
"""


@require_roles(*ALL_AUTHORIZED)
async def help_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.effective_message.reply_text(HELP_TEXT)
