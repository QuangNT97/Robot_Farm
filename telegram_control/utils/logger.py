from __future__ import annotations

import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

from telegram_control.config import BASE_DIR


def setup_logging() -> logging.Logger:
    log_dir = BASE_DIR / "data" / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    fmt = "%(asctime)s | %(levelname)s | %(name)s | %(message)s"
    logging.basicConfig(level=logging.INFO, format=fmt)
    handler = RotatingFileHandler(log_dir / "bot.log", maxBytes=1_000_000, backupCount=5, encoding="utf-8")
    handler.setFormatter(logging.Formatter(fmt))
    root = logging.getLogger()
    if not any(isinstance(h, RotatingFileHandler) for h in root.handlers):
        root.addHandler(handler)
    return logging.getLogger("telegram_control")


logger = setup_logging()
