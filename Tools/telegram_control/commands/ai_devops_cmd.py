from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.ai_devops_service import (
    apply_approved_task,
    create_code_task,
    list_ai_tasks,
    read_task_text,
    reject_task,
    run_shell,
)
from telegram_control.services.build_service import run_build
from telegram_control.utils.auth import ADMIN_ONLY, ADMIN_OPERATOR, ALL_AUTHORIZED, require_roles


@require_roles(*ADMIN_OPERATOR)
async def code_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    content = " ".join(context.args).strip()
    if not content:
        await update.effective_message.reply_text("Cú pháp: /code <yêu cầu lập trình>")
        return
    task = create_code_task(update, content)
    await update.effective_message.reply_text(
        f"✅ Đã tạo AI task {task['task_id']}\nWorker sẽ sinh plan rồi gửi lên group để bạn /approve hoặc /reject."
    )


@require_roles(*ALL_AUTHORIZED)
async def tasks_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.effective_message.reply_text(list_ai_tasks()[:3900])


@require_roles(*ALL_AUTHORIZED)
async def task_log_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    if not context.args:
        await update.effective_message.reply_text("Cú pháp: /log <task_id>")
        return
    path, text = read_task_text(context.args[0].strip(), "log")
    if path and len(text) > 3500:
        await update.effective_message.reply_document(document=path.open("rb"), caption=f"Log {path.name}")
    else:
        await update.effective_message.reply_text(text[:3900])


@require_roles(*ALL_AUTHORIZED)
async def task_diff_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    if not context.args:
        await update.effective_message.reply_text("Cú pháp: /diff <task_id>")
        return
    path, text = read_task_text(context.args[0].strip(), "diff")
    if path and len(text) > 3500:
        await update.effective_message.reply_document(document=path.open("rb"), caption=f"Diff {path.name}")
    else:
        await update.effective_message.reply_text(text[:3900])


async def approve_ai_task_if_needed(update: Update, context: ContextTypes.DEFAULT_TYPE, task_id: str) -> bool:
    if not task_id.startswith("TASK_"):
        return False
    settings = context.application.bot_data["settings"]
    await update.effective_message.reply_text(f"✅ Đã approve {task_id}. AI bắt đầu sửa code/build/commit/push devQuang...")
    result = await apply_approved_task(settings, task_id)
    await update.effective_message.reply_text(result[:3900])
    return True


async def reject_ai_task_if_needed(update: Update, context: ContextTypes.DEFAULT_TYPE, task_id: str, reason: str) -> bool:
    if not task_id.startswith("TASK_"):
        return False
    await update.effective_message.reply_text(reject_task(task_id, reason)[:3900])
    return True


@require_roles(*ADMIN_OPERATOR)
async def build_now_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    result = await run_build(settings)
    text = f"Build {'OK' if result.ok else 'LỖI'}\nLog: {result.log_file}\n\n{(result.stdout or result.stderr)[-3000:]}"
    await update.effective_message.reply_text(text[:3900])


@require_roles(*ALL_AUTHORIZED)
async def git_status_now_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    result = await run_shell("git status --short --branch", settings.project_root)
    await update.effective_message.reply_text((result.stdout or result.stderr or "(không có output)")[:3900])


@require_roles(*ADMIN_ONLY)
async def git_push_now_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    branch = settings.auto_push_branch
    current = (await run_shell("git branch --show-current", settings.project_root)).stdout.strip()
    if current != branch:
        await run_shell(f"git checkout {branch}", settings.project_root)
    result = await run_shell(f"git push origin {branch}", settings.project_root)
    await update.effective_message.reply_text((result.stdout + result.stderr)[:3900])