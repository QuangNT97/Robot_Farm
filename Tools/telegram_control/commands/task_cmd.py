from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.task_service import create_task
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles


@require_roles(*ADMIN_OPERATOR)
async def task_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    content = " ".join(context.args).strip()
    if not content:
        await update.effective_message.reply_text("Cú pháp: /task <nội dung yêu cầu>")
        return
    task = create_task(update, content)
    await update.effective_message.reply_text(f"Đã tạo task {task['task_id']}")
