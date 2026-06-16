from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.approval_service import create_approval, format_approval_request
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles


@require_roles(*ADMIN_OPERATOR)
async def gitstatus_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    approval = create_approval(
        update,
        action="git status project",
        command="git status --short --branch",
        risk="đọc trạng thái Git; không thay đổi file nhưng vẫn là shell command cần duyệt",
        payload={"type": "gitstatus"},
    )
    await update.effective_message.reply_text(format_approval_request(approval))
