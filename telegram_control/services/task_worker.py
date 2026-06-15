from __future__ import annotations

import asyncio
import os
import shutil
from pathlib import Path

from telegram.ext import Application

from telegram_control.services.task_service import TASK_DIRS, write_task_result
from telegram_control.utils.file_utils import read_json, write_json
from telegram_control.utils.logger import logger


WORKER_TASK_KEY = "task_worker_task"


def _result_chat_id(settings) -> int | None:
    return settings.allowed_group_id or settings.allowed_chat_id


async def start_task_worker(app: Application) -> None:
    settings = app.bot_data["settings"]
    if not settings.auto_task_worker_enabled:
        logger.info("Auto task worker disabled")
        return
    if app.bot_data.get(WORKER_TASK_KEY):
        return
    app.bot_data[WORKER_TASK_KEY] = asyncio.create_task(_worker_loop(app), name="telegram-task-worker")
    logger.info("Auto task worker started")


async def stop_task_worker(app: Application) -> None:
    task: asyncio.Task | None = app.bot_data.get(WORKER_TASK_KEY)
    if task is None:
        return
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass
    logger.info("Auto task worker stopped")


async def _worker_loop(app: Application) -> None:
    settings = app.bot_data["settings"]
    while True:
        try:
            await _process_next_pending_task(app)
        except asyncio.CancelledError:
            raise
        except Exception:
            logger.exception("Task worker iteration failed")
        await asyncio.sleep(max(1, settings.task_poll_seconds))


async def _process_next_pending_task(app: Application) -> None:
    pending_files = sorted(TASK_DIRS["pending"].glob("TASK_*.json"), key=lambda p: p.stat().st_mtime)
    if not pending_files:
        return

    task_file = None
    task = None
    for candidate in pending_files:
        candidate_data = read_json(candidate)
        if candidate_data.get("type") == "ai_code":
            continue
        task_file = candidate
        task = candidate_data
        break
    if task_file is None or task is None:
        return
    task_id = task["task_id"]
    running_file = TASK_DIRS["running"] / task_file.name

    task["status"] = "running"
    write_json(running_file, task)
    task_file.unlink(missing_ok=True)

    settings = app.bot_data["settings"]
    chat_id = _result_chat_id(settings)
    if chat_id:
        await app.bot.send_message(chat_id=chat_id, text=f"🤖 Bắt đầu xử lý task {task_id}\n\n{task.get('request', '')[:1500]}")

    success = False
    summary = ""
    try:
        success, summary = await _execute_task(settings, running_file, task)
        result_path = write_task_result(task_id, success, summary)

        final_folder = TASK_DIRS["done"] if success else TASK_DIRS["failed"]
        task["status"] = "done" if success else "failed"
        write_json(final_folder / running_file.name, task)
        running_file.unlink(missing_ok=True)

        if success:
            push_summary = await _commit_and_push_if_needed(settings, task_id)
            if push_summary:
                summary = f"{summary}\n\n## Git\n{push_summary}"
                result_path.write_text(summary, encoding="utf-8")

        if chat_id:
            icon = "✅" if success else "❌"
            await app.bot.send_message(
                chat_id=chat_id,
                text=f"{icon} Task {task_id} {'hoàn thành' if success else 'lỗi'}\n\n{summary[:3500]}\n\nDùng /result để xem chi tiết.",
            )
    except Exception as exc:
        logger.exception("Task execution failed: %s", task_id)
        summary = f"# Task {task_id} failed\n\nException: {exc}"
        write_task_result(task_id, False, summary)
        task["status"] = "failed"
        write_json(TASK_DIRS["failed"] / running_file.name, task)
        running_file.unlink(missing_ok=True)
        if chat_id:
            await app.bot.send_message(chat_id=chat_id, text=f"❌ Task {task_id} lỗi\n\n{summary}")


async def _execute_task(settings, task_file: Path, task: dict) -> tuple[bool, str]:
    task_id = task["task_id"]
    request = task.get("request", "")
    if not settings.cline_task_command:
        return False, (
            f"# Task {task_id} chưa được thực thi\n\n"
            "Bot đã tự phát hiện task và chuyển sang worker, nhưng chưa có `CLINE_TASK_COMMAND` trong `.env`.\n\n"
            "Để bot tự gọi agent/Cline, hãy cấu hình một command local đáng tin cậy nhận các biến môi trường:\n\n"
            "- `TASK_ID`\n- `TASK_FILE`\n- `TASK_REQUEST`\n- `PROJECT_ROOT`\n\n"
            "Ví dụ placeholder:\n\n"
            "```env\nCLINE_TASK_COMMAND=python telegram_control/services/local_task_executor.py\n```\n\n"
            f"Nội dung task:\n\n```text\n{request}\n```"
        )

    env = os.environ.copy()
    env.update({
        "TASK_ID": task_id,
        "TASK_FILE": str(task_file),
        "TASK_REQUEST": request,
        "PROJECT_ROOT": str(settings.project_root),
    })
    proc = await asyncio.create_subprocess_shell(
        settings.cline_task_command,
        cwd=str(settings.project_root),
        env=env,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    stdout_b, stderr_b = await asyncio.wait_for(proc.communicate(), timeout=settings.command_timeout_seconds)
    stdout = stdout_b.decode("utf-8", errors="replace")
    stderr = stderr_b.decode("utf-8", errors="replace")
    ok = proc.returncode == 0
    summary = (
        f"# Task {task_id} {'completed' if ok else 'failed'}\n\n"
        f"Request:\n```text\n{request}\n```\n\n"
        f"Command:\n```text\n{settings.cline_task_command}\n```\n\n"
        f"Return code: {proc.returncode}\n\n"
        f"## STDOUT\n```text\n{stdout[-2500:]}\n```\n\n"
        f"## STDERR\n```text\n{stderr[-2500:]}\n```"
    )
    return ok, summary


async def _commit_and_push_if_needed(settings, task_id: str) -> str:
    status = await _run_git("git status --porcelain", settings.project_root)
    if not status.strip():
        return "Không có thay đổi code để commit/push."

    await _run_git("git add .", settings.project_root)
    commit = await _run_git(f'git commit -m "Process Telegram task {task_id}"', settings.project_root)
    push = await _run_git(f"git push origin {settings.auto_push_branch}", settings.project_root)
    return f"Đã commit/push lên `{settings.auto_push_branch}`.\n\n```text\n{commit[-1200:]}\n{push[-1200:]}\n```"


async def _run_git(command: str, cwd: Path) -> str:
    proc = await asyncio.create_subprocess_shell(
        command,
        cwd=str(cwd),
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    stdout_b, stderr_b = await proc.communicate()
    return (stdout_b + stderr_b).decode("utf-8", errors="replace")