from __future__ import annotations

"""Local CLI executor for Telegram tasks.

This is a small, safe executor that can run without external AI API keys.
It reads task details from environment variables provided by task_worker.py:

- TASK_ID
- TASK_REQUEST
- PROJECT_ROOT

It intentionally supports only known safe task patterns. For open-ended AI coding,
replace CLINE_TASK_COMMAND with a real AI/agent CLI later.
"""

import os
from pathlib import Path


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _opencv_camera_code() -> str:
    return '''"""Simple OpenCV camera preview.

Run:
    python Project_Farm_Here/opencv_camera.py

Press `q` or `Esc` to close the camera window.
"""

from __future__ import annotations

import argparse

import cv2


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Display a webcam preview using OpenCV.")
    parser.add_argument("--camera-index", type=int, default=0, help="Camera device index.")
    parser.add_argument("--width", type=int, default=640, help="Requested preview width.")
    parser.add_argument("--height", type=int, default=480, help="Requested preview height.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    cap = cv2.VideoCapture(args.camera_index)
    if not cap.isOpened():
        print(f"ERROR: Cannot open camera index {args.camera_index}")
        return 1

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)

    window_name = "RoboLife Farm - OpenCV Camera"
    print("Camera opened. Press 'q' or Esc to exit.")

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("ERROR: Cannot read frame from camera")
                return 1
            cv2.imshow(window_name, frame)
            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), 27):
                break
    finally:
        cap.release()
        cv2.destroyAllWindows()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
'''


def _handle_opencv_camera(project_root: Path) -> str:
    target_dir = project_root / "Project_Farm_Here"
    _write(target_dir / "opencv_camera.py", _opencv_camera_code())
    _write(target_dir / "requirements.txt", "opencv-python>=4.8.0\n")
    return """Created/updated OpenCV camera demo.

Files:
- Project_Farm_Here/opencv_camera.py
- Project_Farm_Here/requirements.txt

Suggested commands:
```bash
pip install -r Project_Farm_Here/requirements.txt
python Project_Farm_Here/opencv_camera.py
```

Telegram controls:
- `/run_project` to run the configured project command.
- `/stop_project` to stop it.
"""


def _handle_generic_task(project_root: Path, task_id: str, request: str) -> str:
    notes_dir = project_root / "telegram_control" / "tasks" / "executor_notes"
    _write(
        notes_dir / f"{task_id}.md",
        f"# Unsupported task pattern\n\nRequest:\n\n```text\n{request}\n```\n\n"
        "This local rule-based executor cannot safely implement this request yet. "
        "Use an AI/API based executor for open-ended coding tasks.\n",
    )
    return """Task was received, but the local rule-based executor does not support this request yet.

I created a note under `telegram_control/tasks/executor_notes/`.

To support fully automatic coding, configure `CLINE_TASK_COMMAND` to call a real AI/agent CLI.
"""


def main() -> int:
    task_id = os.getenv("TASK_ID", "TASK_UNKNOWN")
    request = os.getenv("TASK_REQUEST", "")
    project_root = Path(os.getenv("PROJECT_ROOT", ".")).resolve()

    req = request.lower()
    if "opencv" in req and ("camera" in req or "hiển thị" in req or "hien thi" in req):
        summary = _handle_opencv_camera(project_root)
    else:
        summary = _handle_generic_task(project_root, task_id, request)

    print(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())