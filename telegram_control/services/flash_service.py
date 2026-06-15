from __future__ import annotations

from telegram_control.config import BASE_DIR
from telegram_control.utils.command_runner import run_fixed_command
from telegram_control.utils.file_utils import now_iso


def _safe_stamp() -> str:
    return now_iso().replace(":", "-").replace("+", "_")


async def run_flash(settings):
    return await run_fixed_command(
        settings.flash_command,
        cwd=settings.project_root,
        timeout=settings.command_timeout_seconds,
        log_file=BASE_DIR / "data" / "logs" / f"flash_{_safe_stamp()}.log",
    )
