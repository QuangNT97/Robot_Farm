from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.project_process_service import run_project, stop_project
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles


@require_roles(*ADMIN_OPERATOR)
async def run_project_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    await update.effective_message.reply_text(await run_project(settings))


@require_roles(*ADMIN_OPERATOR)
async def stop_project_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.effective_message.reply_text(await stop_project())