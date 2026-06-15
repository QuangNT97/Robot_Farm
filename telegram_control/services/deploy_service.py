from __future__ import annotations

from telegram_control.services.build_service import run_build
from telegram_control.services.flash_service import run_flash
from telegram_control.services.serial_service import read_serial_log
from telegram_control.utils.file_utils import tail_text


async def run_deploy(settings) -> tuple[bool, str]:
    build = await run_build(settings)
    if not build.ok:
        return False, f"Build lỗi. Log: {build.log_file}\n\n{tail_text(build.log_file, settings.log_tail_lines)}"
    flash = await run_flash(settings)
    if not flash.ok:
        return False, f"Flash lỗi. Log: {flash.log_file}\n\n{tail_text(flash.log_file, settings.log_tail_lines)}"
    serial_path = await read_serial_log(settings)
    return True, (
        "Deploy hoàn tất.\n"
        f"Build log: {build.log_file}\n"
        f"Flash log: {flash.log_file}\n"
        f"Serial log: {serial_path}\n\n"
        f"Serial tail:\n{tail_text(serial_path, settings.log_tail_lines)}"
    )
