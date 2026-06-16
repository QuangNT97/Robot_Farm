from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes
from telegram_control.config import BASE_DIR
from telegram_control.utils.auth import ALL_AUTHORIZED, require_roles
from telegram_control.utils.file_utils import tail_text


@require_roles(*ALL_AUTHORIZED)
async def logs_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    log_path = BASE_DIR / "data" / "logs" / "bot.log"
    text = tail_text(log_path, settings.log_tail_lines) or "Chưa có log hệ thống."
    await update.effective_message.reply_text(text[:3900])
