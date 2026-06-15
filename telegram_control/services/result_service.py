from __future__ import annotations

from pathlib import Path

from telegram_control.config import BASE_DIR


def summarize_from_available_data(request: str, max_lines: int = 80) -> str:
    """Rule-based summary only from existing logs/results; no fabricated content."""
    sources: list[Path] = []
    sources.extend(sorted((BASE_DIR / "data" / "logs").glob("*.log"), key=lambda p: p.stat().st_mtime, reverse=True)[:5])
    sources.extend(sorted((BASE_DIR / "tasks" / "done").glob("*_result.md"), key=lambda p: p.stat().st_mtime, reverse=True)[:3])
    sources.extend(sorted((BASE_DIR / "tasks" / "failed").glob("*_error.md"), key=lambda p: p.stat().st_mtime, reverse=True)[:3])
    if not sources:
        return "Thiếu dữ liệu: chưa có log, build output, flash output, serial output hoặc result file."

    req = request.lower()
    combined: list[str] = []
    for path in sources:
        text = path.read_text(encoding="utf-8", errors="replace").splitlines()
        if "20 dòng" in req or "20 dong" in req:
            selected = text[-20:]
        elif "lỗi" in req or "loi" in req or "error" in req:
            selected = [line for line in text if any(k in line.lower() for k in ["error", "failed", "lỗi", "exception", "traceback"])] or text[-max_lines:]
        else:
            selected = text[-max_lines:]
        combined.append(f"## {path.name}\n" + "\n".join(selected))
    return "Tổng hợp theo dữ liệu hiện có, không bịa kết quả:\n\n" + "\n\n".join(combined)
