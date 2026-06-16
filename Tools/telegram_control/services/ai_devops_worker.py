from __future__ import annotations

import asyncio

from telegram.ext import Application

from telegram_control.services.ai_devops_service import AI_LOG_DIR, AI_TASK_DIRS, generate_plan, read_task_text
from telegram_control.utils.logger import logger

KEY = "ai_devops_worker"


async def start_ai_devops_worker(app: Application) -> None:
    settings = app.bot_data["settings"]
    if not settings.ai_devops_enabled:
        return
    app.bot_data[KEY] = asyncio.create_task(_loop(app), name="ai-devops-worker")


async def stop_ai_devops_worker(app: Application) -> None:
    task = app.bot_data.get(KEY)
    if task:
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass


async def _loop(app: Application) -> None:
    settings = app.bot_data["settings"]
    while True:
        try:
            files = sorted(AI_TASK_DIRS["pending"].glob("TASK_*.json"), key=lambda p: p.stat().st_mtime)
            for path in files:
                task = await generate_plan(settings, path)
                chat_id = task.get("chat_id") or settings.allowed_group_id or settings.allowed_chat_id
                _, plan_text = read_task_text(task["task_id"], "plan")
                if chat_id:
                    await app.bot.send_message(chat_id=chat_id, text=(f"🧠 AI plan ready for {task['task_id']}\n\n{plan_text[:3300]}\n\nApprove: /approve {task['task_id']}\nReject: /reject {task['task_id']} lý do"))
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            logger.exception("AI DevOps worker failed")
            (AI_LOG_DIR / "ai_devops_worker_error.log").write_text(str(exc), encoding="utf-8")
            chat_id = settings.allowed_group_id or settings.allowed_chat_id
            if chat_id:
                await app.bot.send_message(
                    chat_id=chat_id,
                    text=f"❌ AI DevOps worker error: {exc}\n\nWorker đã dừng để tránh spam Telegram. Hãy sửa lỗi rồi khởi động lại bot.",
                )
            return
        await asyncio.sleep(max(1, settings.ai_task_poll_seconds))