from __future__ import annotations

import asyncio
import time
from pathlib import Path

import cv2

from telegram_control.config import BASE_DIR
from telegram_control.utils.file_utils import now_iso


class CameraError(RuntimeError):
    pass


def _timestamp_name(prefix: str, suffix: str) -> Path:
    safe = now_iso().replace(":", "-").replace("+", "_")
    return BASE_DIR / "data" / prefix / f"{prefix}_{safe}.{suffix}"


def _capture_photo_sync(camera_index: int = 0) -> Path:
    path = _timestamp_name("photos", "jpg")
    cap = cv2.VideoCapture(camera_index)
    try:
        if not cap.isOpened():
            raise CameraError("Không mở được camera laptop.")
        ok, frame = cap.read()
        if not ok:
            raise CameraError("Không đọc được frame từ camera.")
        cv2.imwrite(str(path), frame)
        return path
    finally:
        cap.release()


def _record_video_sync(seconds: int, camera_index: int = 0) -> Path:
    path = _timestamp_name("videos", "mp4")
    cap = cv2.VideoCapture(camera_index)
    try:
        if not cap.isOpened():
            raise CameraError("Không mở được camera laptop.")
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)) or 640
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)) or 480
        fps = cap.get(cv2.CAP_PROP_FPS) or 20.0
        writer = cv2.VideoWriter(str(path), cv2.VideoWriter_fourcc(*"mp4v"), fps, (width, height))
        if not writer.isOpened():
            raise CameraError("Không tạo được file video.")
        end = time.time() + seconds
        while time.time() < end:
            ok, frame = cap.read()
            if not ok:
                raise CameraError("Camera bị lỗi khi đang quay video.")
            writer.write(frame)
        writer.release()
        return path
    finally:
        cap.release()


async def capture_photo(camera_index: int = 0) -> Path:
    return await asyncio.to_thread(_capture_photo_sync, camera_index)


async def record_video(seconds: int, camera_index: int = 0) -> Path:
    return await asyncio.to_thread(_record_video_sync, seconds, camera_index)
