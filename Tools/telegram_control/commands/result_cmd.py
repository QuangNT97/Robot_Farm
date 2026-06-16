from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes
from telegram_control.services.result_service import summarize_from_available_data
from telegram_control.services.task_service import read_latest_result
from telegram_control.utils.auth import ALL_AUTHORIZED, require_roles


@require_roles(*ALL_AUTHORIZED)
async def result_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    path, text = read_latest_result()
    if path is None:
        await update.effective_message.reply_text(text)
        return
    if len(text) > 3500:
        await update.effective_message.reply_document(document=path.open("rb"), caption=f"Result: {path.name}")
    else:
        await update.effective_message.reply_text(f"Result: {path.name}\n\n{text}")


@require_roles(*ALL_AUTHORIZED)
async def ask_result_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    request = " ".join(context.args).strip()
    if not request:
        await update.effective_message.reply_text("Cú pháp: /ask_result <yêu cầu nội dung trả về>")
        return
    settings = context.application.bot_data["settings"]
    text = summarize_from_available_data(request, settings.log_tail_lines)
    await update.effective_message.reply_text(text[:3900] if len(text) > 3900 else text)
