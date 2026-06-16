from __future__ import annotations

import uuid
from pathlib import Path

from telegram import Update

from telegram_control.config import BASE_DIR
from telegram_control.utils.file_utils import now_iso, read_json, write_json


TASK_DIRS = {
    "pending": BASE_DIR / "tasks" / "pending",
    "running": BASE_DIR / "tasks" / "running",
    "waiting_approval": BASE_DIR / "tasks" / "waiting_approval",
    "done": BASE_DIR / "tasks" / "done",
    "failed": BASE_DIR / "tasks" / "failed",
}


def create_task(update: Update, content: str) -> dict:
    task_id = f"TASK_{uuid.uuid4().hex[:10].upper()}"
    user = update.effective_user
    chat = update.effective_chat
    data = {
        "task_id": task_id,
        "created_at": now_iso(),
        "created_by": {
            "user_id": user.id if user else None,
            "username": user.username if user else None,
            "full_name": user.full_name if user else None,
        },
        "chat_id": chat.id if chat else None,
        "request": content,
        "status": "pending",
    }
    write_json(TASK_DIRS["pending"] / f"{task_id}.json", data)
    return data


def latest_result_file() -> Path | None:
    files = list((BASE_DIR / "tasks" / "done").glob("*_result.md")) + list((BASE_DIR / "tasks" / "failed").glob("*_error.md"))
    return max(files, key=lambda p: p.stat().st_mtime) if files else None


def read_latest_result() -> tuple[Path | None, str]:
    path = latest_result_file()
    if path is None:
        return None, "Chưa có result file nào."
    return path, path.read_text(encoding="utf-8", errors="replace")


def write_task_result(task_id: str, success: bool, content: str) -> Path:
    folder = TASK_DIRS["done"] if success else TASK_DIRS["failed"]
    suffix = "result" if success else "error"
    path = folder / f"{task_id}_{suffix}.md"
    path.write_text(content, encoding="utf-8")
    return path
