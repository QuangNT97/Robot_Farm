from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.approval_service import create_approval, format_approval_request
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles


@require_roles(*ADMIN_OPERATOR)
async def deploy_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    approval = create_approval(
        update,
        action="deploy: build + flash + read serial",
        command=f"BUILD: {settings.build_command} | FLASH: {settings.flash_command} | SERIAL: {settings.serial_port}",
        risk="ảnh hưởng firmware/hardware, có thể lỗi build/flash hoặc treo serial",
        payload={"type": "deploy"},
    )
    await update.effective_message.reply_text(format_approval_request(approval))
