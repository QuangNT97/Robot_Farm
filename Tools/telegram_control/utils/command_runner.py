from __future__ import annotations

import asyncio
from dataclasses import dataclass
from pathlib import Path

from telegram_control.utils.file_utils import ensure_parent, now_iso


@dataclass
class CommandResult:
    command: str
    returncode: int
    stdout: str
    stderr: str
    log_file: Path

    @property
    def ok(self) -> bool:
        return self.returncode == 0


async def run_fixed_command(command: str, cwd: Path, timeout: int, log_file: Path) -> CommandResult:
    """Run a trusted command from config only. Never pass user raw shell input here."""
    if not command:
        raise ValueError("Command chưa được cấu hình trong .env")
    ensure_parent(log_file)
    proc = await asyncio.create_subprocess_shell(
        command,
        cwd=str(cwd),
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    timed_out = False
    try:
        stdout_b, stderr_b = await asyncio.wait_for(proc.communicate(), timeout=timeout)
    except asyncio.TimeoutError:
        timed_out = True
        proc.kill()
        stdout_b, stderr_b = await proc.communicate()
        stderr_b += f"\nTIMEOUT after {timeout}s".encode()
    stdout = stdout_b.decode("utf-8", errors="replace")
    stderr = stderr_b.decode("utf-8", errors="replace")
    text = f"# Command log\nTime: {now_iso()}\nCommand: {command}\nReturn code: {proc.returncode}\n\n## STDOUT\n{stdout}\n\n## STDERR\n{stderr}\n"
    log_file.write_text(text, encoding="utf-8")
    return CommandResult(command, -1 if timed_out else (proc.returncode or 0), stdout, stderr, log_file)
