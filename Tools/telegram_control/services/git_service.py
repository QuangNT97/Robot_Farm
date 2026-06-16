from __future__ import annotations

from pathlib import Path


from telegram_control.config import BASE_DIR
from telegram_control.utils.command_runner import run_fixed_command


async def git_status(settings):
    return await run_fixed_command(
        "git status --short --branch",
        cwd=settings.project_root,
        timeout=60,
        log_file=BASE_DIR / "data" / "logs" / "gitstatus.log",
    )
