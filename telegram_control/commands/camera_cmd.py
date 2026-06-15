from __future__ import annotations

from telegram import Update
from telegram.ext import ContextTypes

from telegram_control.services.camera_service import CameraError, capture_photo, record_video
from telegram_control.utils.auth import ADMIN_OPERATOR, require_roles
from telegram_control.utils.logger import logger


@require_roles(*ADMIN_OPERATOR)
async def cam10_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    settings = context.application.bot_data["settings"]
    await update.effective_message.reply_text(f"Đang quay video {settings.default_camera_seconds} giây...")
    try:
        path = await record_video(settings.default_camera_seconds)
        await update.effective_message.reply_video(video=path.open("rb"), caption=f"Video: {path.name}")
    except CameraError as exc:
        logger.exception("Camera video error")
        await update.effective_message.reply_text(f"❌ Lỗi camera: {exc}")


@require_roles(*ADMIN_OPERATOR)
async def photo_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    try:
        path = await capture_photo()
        await update.effective_message.reply_photo(photo=path.open("rb"), caption=f"Photo: {path.name}")
    except CameraError as exc:
        logger.exception("Camera photo error")
        await update.effective_message.reply_text(f"❌ Lỗi camera: {exc}")
