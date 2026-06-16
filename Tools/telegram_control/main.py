from __future__ import annotations

from telegram.ext import Application

from telegram_control.commands import register_commands
from telegram_control.config import settings
from telegram_control.services.ai_devops_worker import start_ai_devops_worker, stop_ai_devops_worker
from telegram_control.services.task_worker import start_task_worker, stop_task_worker
from telegram_control.utils.logger import logger


async def _post_init(app: Application) -> None:
    await start_task_worker(app)
    await start_ai_devops_worker(app)


async def _post_shutdown(app: Application) -> None:
    await stop_ai_devops_worker(app)
    await stop_task_worker(app)


def build_app() -> Application:
    if not settings.bot_token:
        raise RuntimeError("BOT_TOKEN chưa được cấu hình. Hãy tạo telegram_control/.env từ .env.example")
    app = Application.builder().token(settings.bot_token).post_init(_post_init).post_shutdown(_post_shutdown).build()
    app.bot_data["settings"] = settings
    register_commands(app)
    return app


def main() -> None:
    logger.info("Starting Farm Robot Telegram Bot")
    app = build_app()
    app.run_polling(allowed_updates=None)


if __name__ == "__main__":
    main()
