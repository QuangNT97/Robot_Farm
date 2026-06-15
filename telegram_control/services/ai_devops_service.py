from __future__ import annotations

import asyncio
import json
import os
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from telegram_control.config import BASE_DIR
from telegram_control.services.build_service import run_build
from telegram_control.utils.file_utils import now_iso, read_json, write_json


AI_ROOT = BASE_DIR / "ai_devops"
AI_TASK_DIRS = {
    "pending": BASE_DIR / "tasks" / "pending",
    "approved": BASE_DIR / "tasks" / "approved",
    "completed": BASE_DIR / "tasks" / "completed",
    "failed": BASE_DIR / "tasks" / "failed",
}
AI_LOG_DIR = BASE_DIR / "logs"

IGNORE_DIRS = {".git", "__pycache__", ".venv", "venv", "node_modules", "data", "logs"}
SOURCE_EXTS = {".py", ".md", ".txt", ".json", ".env.example", ".bat", ".sh", ".toml", ".yaml", ".yml", ".ino", ".cpp", ".h", ".hpp", ".c"}


@dataclass
class ShellResult:
    returncode: int
    stdout: str
    stderr: str


def ensure_ai_dirs() -> None:
    for folder in [*AI_TASK_DIRS.values(), AI_LOG_DIR, AI_ROOT]:
        folder.mkdir(parents=True, exist_ok=True)


def create_code_task(update, content: str) -> dict[str, Any]:
    ensure_ai_dirs()
    task_id = f"TASK_{uuid.uuid4().hex[:10].upper()}"
    data = {
        "task_id": task_id,
        "type": "ai_code",
        "status": "pending_plan",
        "created_at": now_iso(),
        "created_by": {
            "user_id": update.effective_user.id if update.effective_user else None,
            "username": update.effective_user.username if update.effective_user else None,
            "full_name": update.effective_user.full_name if update.effective_user else None,
        },
        "chat_id": update.effective_chat.id if update.effective_chat else None,
        "request": content,
    }
    write_json(AI_TASK_DIRS["pending"] / f"{task_id}.json", data)
    return data


def list_ai_tasks() -> str:
    ensure_ai_dirs()
    lines = ["📋 AI DevOps tasks:"]
    for name, folder in AI_TASK_DIRS.items():
        files = sorted(folder.glob("TASK_*.json"), key=lambda p: p.stat().st_mtime, reverse=True)[:10]
        lines.append(f"\n{name}: {len(files)}")
        for path in files[:5]:
            data = read_json(path)
            lines.append(f"- {data.get('task_id')} | {data.get('status')} | {data.get('request','')[:60]}")
    return "\n".join(lines)


def find_task_file(task_id: str) -> Path | None:
    for folder in AI_TASK_DIRS.values():
        path = folder / f"{task_id}.json"
        if path.exists():
            return path
    return None


def read_task_text(task_id: str, kind: str) -> tuple[Path | None, str]:
    suffix = {"log": "log.md", "diff": "diff.patch", "plan": "plan.md"}.get(kind, kind)
    matches = list(BASE_DIR.glob(f"tasks/**/{task_id}_{suffix}")) + list(AI_LOG_DIR.glob(f"{task_id}_*.log"))
    if not matches:
        return None, f"Không tìm thấy {kind} cho {task_id}."
    path = max(matches, key=lambda p: p.stat().st_mtime)
    return path, path.read_text(encoding="utf-8", errors="replace")


async def run_shell(command: str, cwd: Path, log_file: Path | None = None) -> ShellResult:
    proc = await asyncio.create_subprocess_shell(command, cwd=str(cwd), stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    out_b, err_b = await proc.communicate()
    out = out_b.decode("utf-8", errors="replace")
    err = err_b.decode("utf-8", errors="replace")
    if log_file:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        log_file.write_text(f"$ {command}\n\n## STDOUT\n{out}\n\n## STDERR\n{err}\n", encoding="utf-8")
    return ShellResult(proc.returncode or 0, out, err)


async def ensure_dev_branch(settings) -> None:
    branch = (await run_shell("git branch --show-current", settings.project_root)).stdout.strip()
    if branch != settings.auto_push_branch:
        await run_shell(f"git checkout {settings.auto_push_branch}", settings.project_root, AI_LOG_DIR / "git_checkout.log")


def collect_source_context(project_root: Path, max_chars: int) -> str:
    chunks: list[str] = []
    used = 0
    for path in sorted(project_root.rglob("*")):
        if not path.is_file() or any(part in IGNORE_DIRS for part in path.parts):
            continue
        if path.name == ".env" or (path.suffix not in SOURCE_EXTS and path.name != ".env.example"):
            continue
        rel = path.relative_to(project_root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        block = f"\n--- FILE: {rel} ---\n{text[:8000]}\n"
        if used + len(block) > max_chars:
            break
        chunks.append(block)
        used += len(block)
    return "".join(chunks)


def _extract_json(text: str) -> dict[str, Any]:
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end < start:
        raise ValueError("AI response does not contain JSON")
    return json.loads(text[start:end + 1])


async def generate_plan(settings, task_path: Path) -> dict[str, Any]:
    task = read_json(task_path)
    if not settings.openai_api_key:
        raise RuntimeError("OPENAI_API_KEY chưa được cấu hình trong .env")
    try:
        from openai import OpenAI
    except ImportError as exc:
        raise RuntimeError("Thiếu thư viện openai. Hãy chạy: pip install -r telegram_control/requirements.txt") from exc
    context = collect_source_context(settings.project_root, settings.max_source_chars)
    prompt = f"""
You are an AI DevOps coding agent for Farm_Robot. Return STRICT JSON only.
Rules: never merge/push main. Only propose changes. User must approve before writing files.

Task: {task['request']}

Source context:
{context}

JSON schema:
{{
  "title": "short task title",
  "plan": ["step 1", "step 2"],
  "summary": "what will change",
  "file_changes": [{{"path":"relative/path", "content":"full new file content", "reason":"why"}}],
  "build_command": "optional build command or empty"
}}
"""
    client = OpenAI(api_key=settings.openai_api_key)
    response = await asyncio.to_thread(
        client.chat.completions.create,
        model=settings.openai_model,
        messages=[{"role": "system", "content": "Return strict JSON only."}, {"role": "user", "content": prompt}],
    )
    data = _extract_json(response.choices[0].message.content or "")
    task.update({"status": "waiting_approval", "ai_plan": data, "planned_at": now_iso()})
    plan_md = _render_plan(task)
    approved_json = AI_TASK_DIRS["approved"] / task_path.name
    plan_path = AI_TASK_DIRS["approved"] / f"{task['task_id']}_plan.md"
    plan_path.write_text(plan_md, encoding="utf-8")
    write_json(approved_json, task)
    task_path.unlink(missing_ok=True)
    return task


def _render_plan(task: dict[str, Any]) -> str:
    plan = task.get("ai_plan", {})
    files = "\n".join(f"- `{f.get('path')}`: {f.get('reason','')}" for f in plan.get("file_changes", []))
    steps = "\n".join(f"{i+1}. {s}" for i, s in enumerate(plan.get("plan", [])))
    return f"# {task['task_id']} - {plan.get('title','AI Task')}\n\n## Request\n{task.get('request')}\n\n## Plan\n{steps}\n\n## Files\n{files}\n\n## Summary\n{plan.get('summary','')}\n"


async def apply_approved_task(settings, task_id: str) -> str:
    path = find_task_file(task_id)
    if path is None:
        return f"Không tìm thấy task {task_id}."
    task = read_json(path)
    if task.get("status") != "waiting_approval":
        return f"Task {task_id} chưa ở trạng thái waiting_approval."
    await ensure_dev_branch(settings)
    plan = task.get("ai_plan", {})
    changed: list[str] = []
    for item in plan.get("file_changes", []):
        rel = str(item["path"]).replace("\\", "/").lstrip("/")
        if rel.startswith(".git/") or rel == ".env":
            raise RuntimeError(f"Unsafe file path: {rel}")
        target = settings.project_root / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(item.get("content", ""), encoding="utf-8")
        changed.append(rel)
    diff = (await run_shell("git diff", settings.project_root)).stdout
    diff_path = AI_TASK_DIRS["approved"] / f"{task_id}_diff.patch"
    diff_path.write_text(diff, encoding="utf-8")
    build = await run_build(settings) if settings.build_command else None
    if build and not build.ok:
        task["status"] = "failed_build"
        failed = AI_TASK_DIRS["failed"] / path.name
        write_json(failed, task)
        path.unlink(missing_ok=True)
        return f"❌ Build lỗi cho {task_id}\nLog: {build.log_file}\n\n{(build.stdout or build.stderr)[-2500:]}"
    title = plan.get("title") or task.get("request", "AI task")[:60]
    await run_shell("git add .", settings.project_root, AI_LOG_DIR / f"{task_id}_git_add.log")
    commit = await run_shell(f'git commit -m "[AI TASK] {title}"', settings.project_root, AI_LOG_DIR / f"{task_id}_git_commit.log")
    if commit.returncode != 0 and "nothing to commit" not in (commit.stdout + commit.stderr).lower():
        raise RuntimeError(commit.stdout + commit.stderr)
    push = await run_shell(f"git push origin {settings.auto_push_branch}", settings.project_root, AI_LOG_DIR / f"{task_id}_git_push.log")
    task.update({"status": "completed", "completed_at": now_iso(), "changed_files": changed})
    completed = AI_TASK_DIRS["completed"] / path.name
    write_json(completed, task)
    path.unlink(missing_ok=True)
    result = f"✅ Task {task_id} completed\n\nFiles changed:\n" + "\n".join(f"- {f}" for f in changed)
    result += f"\n\nPushed to `{settings.auto_push_branch}`."
    (AI_TASK_DIRS["completed"] / f"{task_id}_log.md").write_text(result, encoding="utf-8")
    return result


def reject_task(task_id: str, reason: str) -> str:
    path = find_task_file(task_id)
    if path is None:
        return f"Không tìm thấy task {task_id}."
    task = read_json(path)
    task.update({"status": "rejected", "rejected_at": now_iso(), "reject_reason": reason})
    failed = AI_TASK_DIRS["failed"] / path.name
    write_json(failed, task)
    path.unlink(missing_ok=True)
    return f"❌ Đã reject {task_id}. Lý do: {reason}"
