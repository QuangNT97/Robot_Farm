from __future__ import annotations

import shutil
import uuid
from pathlib import Path
from typing import Any

from telegram import Update

from telegram_control.config import BASE_DIR
from telegram_control.utils.file_utils import now_iso, read_json, write_json


PENDING = BASE_DIR / "approvals" / "pending"
APPROVED = BASE_DIR / "approvals" / "approved"
REJECTED = BASE_DIR / "approvals" / "rejected"


def create_approval(update: Update, action: str, risk: str, command: str | None = None, task_id: str | None = None, payload: dict[str, Any] | None = None) -> dict:
    approval_id = f"APPROVAL_{uuid.uuid4().hex[:10].upper()}"
    user = update.effective_user
    chat = update.effective_chat
    data = {
        "approval_id": approval_id,
        "task_id": task_id,
        "created_at": now_iso(),
        "requested_by": {
            "user_id": user.id if user else None,
            "username": user.username if user else None,
            "full_name": user.full_name if user else None,
        },
        "chat_id": chat.id if chat else None,
        "action": action,
        "command": command,
        "risk": risk,
        "status": "waiting",
        "payload": payload or {},
    }
    write_json(PENDING / f"{approval_id}.json", data)
    return data


def get_pending(approval_id: str) -> dict | None:
    path = PENDING / f"{approval_id}.json"
    return read_json(path) if path.exists() else None


def approve(approval_id: str, admin_user_id: int) -> dict | None:
    src = PENDING / f"{approval_id}.json"
    if not src.exists():
        return None
    data = read_json(src)
    data["status"] = "approved"
    data["approved_at"] = now_iso()
    data["approved_by"] = admin_user_id
    dst = APPROVED / src.name
    write_json(dst, data)
    src.unlink()
    return data


def reject(approval_id: str, admin_user_id: int, reason: str) -> dict | None:
    src = PENDING / f"{approval_id}.json"
    if not src.exists():
        return None
    data = read_json(src)
    data["status"] = "rejected"
    data["rejected_at"] = now_iso()
    data["rejected_by"] = admin_user_id
    data["reject_reason"] = reason
    dst = REJECTED / src.name
    write_json(dst, data)
    src.unlink()
    return data


def format_approval_request(data: dict) -> str:
    return (
        "Yêu cầu xác nhận:\n"
        f"Approval ID: {data['approval_id']}\n"
        f"Task: {data.get('task_id') or '-'}\n"
        f"Hành động: {data['action']}\n"
        f"Command: {data.get('command') or '-'}\n"
        f"Rủi ro: {data['risk']}\n\n"
        f"ADMIN xác nhận: /approve {data['approval_id']}\n"
        f"Hoặc từ chối: /reject {data['approval_id']} lý do"
    )
