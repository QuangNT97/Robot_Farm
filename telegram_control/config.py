from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import Set

from dotenv import load_dotenv


BASE_DIR = Path(__file__).resolve().parent
load_dotenv(BASE_DIR / ".env")


def _csv_ints(name: str) -> Set[int]:
    raw = os.getenv(name, "").strip()
    if not raw:
        return set()
    return {int(x.strip()) for x in raw.split(",") if x.strip()}


def _int(name: str, default: int) -> int:
    value = os.getenv(name, "").strip()
    return int(value) if value else default


@dataclass(frozen=True)
class Settings:
    bot_token: str
    allowed_chat_id: int | None
    allowed_group_id: int | None
    admin_user_ids: Set[int]
    operator_user_ids: Set[int]
    viewer_user_ids: Set[int]
    project_root: Path
    default_camera_seconds: int
    build_command: str
    flash_command: str
    serial_port: str
    serial_baudrate: int
    command_timeout_seconds: int
    serial_read_seconds: int
    log_tail_lines: int
    auto_task_worker_enabled: bool
    task_poll_seconds: int
    cline_task_command: str
    auto_push_branch: str
    run_project_command: str
    openai_api_key: str
    openai_model: str
    ai_devops_enabled: bool
    ai_task_poll_seconds: int
    max_source_chars: int


def get_settings() -> Settings:
    token = os.getenv("BOT_TOKEN", "").strip()
    return Settings(
        bot_token=token,
        allowed_chat_id=_int("ALLOWED_CHAT_ID", 0) or None,
        allowed_group_id=_int("ALLOWED_GROUP_ID", 0) or None,
        admin_user_ids=_csv_ints("ADMIN_USER_IDS"),
        operator_user_ids=_csv_ints("OPERATOR_USER_IDS"),
        viewer_user_ids=_csv_ints("VIEWER_USER_IDS"),
        project_root=Path(os.getenv("PROJECT_ROOT", str(BASE_DIR.parent))).resolve(),
        default_camera_seconds=_int("DEFAULT_CAMERA_SECONDS", 10),
        build_command=os.getenv("BUILD_COMMAND", "").strip(),
        flash_command=os.getenv("FLASH_COMMAND", "").strip(),
        serial_port=os.getenv("SERIAL_PORT", "").strip(),
        serial_baudrate=_int("SERIAL_BAUDRATE", 115200),
        command_timeout_seconds=_int("COMMAND_TIMEOUT_SECONDS", 300),
        serial_read_seconds=_int("SERIAL_READ_SECONDS", 10),
        log_tail_lines=_int("LOG_TAIL_LINES", 80),
        auto_task_worker_enabled=_int("AUTO_TASK_WORKER_ENABLED", 1) == 1,
        task_poll_seconds=_int("TASK_POLL_SECONDS", 5),
        cline_task_command=os.getenv("CLINE_TASK_COMMAND", "").strip(),
        auto_push_branch=os.getenv("AUTO_PUSH_BRANCH", "devQuang").strip() or "devQuang",
        run_project_command=os.getenv("RUN_PROJECT_COMMAND", "python Project_Farm_Here/opencv_camera.py").strip(),
        openai_api_key=os.getenv("OPENAI_API_KEY", "").strip(),
        openai_model=os.getenv("OPENAI_MODEL", "gpt-5.5").strip() or "gpt-5.5",
        ai_devops_enabled=_int("AI_DEVOPS_ENABLED", 1) == 1,
        ai_task_poll_seconds=_int("AI_TASK_POLL_SECONDS", 5),
        max_source_chars=_int("MAX_SOURCE_CHARS", 120000),
    )


settings = get_settings()
