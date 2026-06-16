from __future__ import annotations

import platform
import sys
from datetime import datetime
from pathlib import Path

from telegram_control.config import BASE_DIR


TASK_DIRS = {
    "pending": BASE_DIR / "tasks" / "pending",
    "running": BASE_DIR / "tasks" / "running",
    "waiting_approval": BASE_DIR / "tasks" / "waiting_approval",
    "done": BASE_DIR / "tasks" / "done",
    "failed": BASE_DIR / "tasks" / "failed",
}


def _count_files(folder: Path, pattern: str) -> int:
    if not folder.exists():
        return 0
    return sum(1 for path in folder.glob(pattern) if path.is_file() and path.name != ".gitkeep")


def get_status(settings) -> str:
    pending = _count_files(TASK_DIRS["pending"], "TASK_*.json")
    running = _count_files(TASK_DIRS["running"], "TASK_*.json")
    waiting_approval = _count_files(TASK_DIRS["waiting_approval"], "TASK_*.json")
    done = _count_files(TASK_DIRS["done"], "TASK_*_result.md")
    failed = _count_files(TASK_DIRS["failed"], "TASK_*_error.md")

    return "\n".join([
        "✅ Bot đang chạy",
        f"Thời gian hiện tại: {datetime.now().astimezone().isoformat(timespec='seconds')}",
        f"Hệ điều hành: {platform.platform()}",
        f"Python version: {sys.version.split()[0]}",
        f"Thư mục project hiện tại: {settings.project_root}",
        "",
        "📌 Task queue:",
        f"- pending: {pending}",
        f"- running: {running}",
        f"- waiting_approval: {waiting_approval}",
        f"- done: {done}",
        f"- failed: {failed}",
        "",
        "Dùng /result để xem result/error mới nhất nếu đã có.",
    ])
