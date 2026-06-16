from __future__ import annotations

import asyncio
import os


_process: asyncio.subprocess.Process | None = None
_command: str | None = None


async def run_project(settings) -> str:
    global _process, _command
    if _process is not None and _process.returncode is None:
        return f"Project đang chạy rồi. Command: `{_command}`"
    if not settings.run_project_command:
        return "RUN_PROJECT_COMMAND chưa được cấu hình trong .env"

    command: str = settings.run_project_command
    _command = command
    _process = await asyncio.create_subprocess_shell(
        command,
        cwd=str(settings.project_root),
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )
    return f"Đã chạy project. PID={_process.pid}\nCommand: `{_command}`"


async def stop_project() -> str:
    global _process, _command
    if _process is None or _process.returncode is not None:
        _process = None
        _command = None
        return "Không có project nào đang chạy bởi bot."

    pid = _process.pid
    if os.name == "nt":
        killer = await asyncio.create_subprocess_exec(
            "taskkill",
            "/PID",
            str(pid),
            "/T",
            "/F",
            stdout=asyncio.subprocess.DEVNULL,
            stderr=asyncio.subprocess.DEVNULL,
        )
        await killer.wait()
    else:
        _process.terminate()
    try:
        await asyncio.wait_for(_process.wait(), timeout=10)
    except asyncio.TimeoutError:
        _process.kill()
        await _process.wait()
    _process = None
    _command = None
    return f"Đã dừng project PID={pid}."