from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.approval_service import create_approval, format_approval_request
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles


@require_roles(*ADMIN_OPERATOR)
async def build_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    command_name = update.effective_message.text.split()[0].lstrip("/").split("@")[0]
    settings = context.application.bot_data["settings"]
    if command_name == "flash":
        approval = create_approval(update, "flash firmware", "có thể ảnh hưởng hardware", settings.flash_command, payload={"type": "flash"})
    else:
        approval = create_approval(update, "build firmware", "có thể mất thời gian hoặc lỗi build", settings.build_command, payload={"type": "build"})
    await update.effective_message.reply_text(format_approval_request(approval))
