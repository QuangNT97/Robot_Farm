from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.utils.auth import ALL_AUTHORIZED, require_roles


@require_roles(*ALL_AUTHORIZED)
async def start_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.effective_message.reply_text("✅ Farm Robot Telegram Bot đã sẵn sàng.")
