from __future__ import annotations

import asyncio
import time
from pathlib import Path

import serial

from telegram_control.config import BASE_DIR
from telegram_control.utils.file_utils import now_iso


def _safe_stamp() -> str:
    return now_iso().replace(":", "-").replace("+", "_")


def _read_serial_sync(port: str, baudrate: int, seconds: int) -> Path:
    if not port:
        raise ValueError("SERIAL_PORT chưa được cấu hình trong .env")
    path = BASE_DIR / "data" / "logs" / f"serial_{_safe_stamp()}.log"
    lines: list[str] = []
    with serial.Serial(port, baudrate=baudrate, timeout=1) as ser:
        end = time.time() + seconds
        while time.time() < end:
            raw = ser.readline()
            if raw:
                lines.append(raw.decode("utf-8", errors="replace").rstrip())
    path.write_text("\n".join(lines), encoding="utf-8")
    return path


async def read_serial_log(settings) -> Path:
    return await asyncio.to_thread(_read_serial_sync, settings.serial_port, settings.serial_baudrate, settings.serial_read_seconds)
