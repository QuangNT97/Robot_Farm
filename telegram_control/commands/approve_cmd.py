from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.approval_service import approve, reject
from telegram_control.services.build_service import run_build
from telegram_control.services.deploy_service import run_deploy
from telegram_control.services.flash_service import run_flash
from telegram_control.services.git_service import git_status
from telegram_control.commands.ai_devops_cmd import approve_ai_task_if_needed, reject_ai_task_if_needed
from telegram_control.utils.auth import ADMIN_ONLY, require_roles
from telegram_control.utils.file_utils import tail_text
from telegram_control.utils.logger import logger


async def _execute_approved_action(update: Update, context: ContextTypes.DEFAULT_TYPE, approval: dict) -> None:
    settings = context.application.bot_data["settings"]
    action_type = approval.get("payload", {}).get("type")
    try:
        if action_type == "build":
            result = await run_build(settings)
            text = f"Build {'OK' if result.ok else 'LỖI'}\nLog: {result.log_file}\n\n{tail_text(result.log_file, settings.log_tail_lines)}"
        elif action_type == "flash":
            result = await run_flash(settings)
            text = f"Flash {'OK' if result.ok else 'LỖI'}\nLog: {result.log_file}\n\n{tail_text(result.log_file, settings.log_tail_lines)}"
        elif action_type == "deploy":
            ok, text = await run_deploy(settings)
            text = ("Deploy OK\n" if ok else "Deploy LỖI\n") + text
        elif action_type == "gitstatus":
            result = await git_status(settings)
            text = f"Git status return code: {result.returncode}\n\n{result.stdout or result.stderr or '(không có output)'}"
        else:
            text = f"Approval đã duyệt nhưng chưa có executor cho type={action_type}"
        await update.effective_message.reply_text(text[:3900])
    except Exception as exc:
        logger.exception("Approved action failed")
        await update.effective_message.reply_text(f"❌ Lỗi khi thực thi approval: {exc}")


@require_roles(*ADMIN_ONLY)
async def approve_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    if not context.args:
        await update.effective_message.reply_text("Cú pháp: /approve APPROVAL_xxx")
        return
    approval_id = context.args[0].strip()
    if await approve_ai_task_if_needed(update, context, approval_id):
        return
    data = approve(approval_id, update.effective_user.id)
    if data is None:
        await update.effective_message.reply_text("Không tìm thấy approval đang chờ.")
        return
    await update.effective_message.reply_text(f"✅ Đã approve {approval_id}. Bắt đầu thực thi...")
    await _execute_approved_action(update, context, data)


@require_roles(*ADMIN_ONLY)
async def reject_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    if not context.args:
        await update.effective_message.reply_text("Cú pháp: /reject APPROVAL_xxx lý do")
        return
    approval_id = context.args[0].strip()
    reason = " ".join(context.args[1:]).strip() or "Không nêu lý do"
    if await reject_ai_task_if_needed(update, context, approval_id, reason):
        return
    data = reject(approval_id, update.effective_user.id, reason)
    if data is None:
        await update.effective_message.reply_text("Không tìm thấy approval đang chờ.")
        return
    await update.effective_message.reply_text(f"❌ Đã reject {approval_id}. Lý do: {reason}")
