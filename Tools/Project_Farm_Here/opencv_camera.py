"""Simple OpenCV grayscale camera preview.

Run:
    python Project_Farm_Here/opencv_camera.py

Press `q` or `Esc` to close the camera window.
"""

from __future__ import annotations

import argparse

import cv2


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Display a grayscale webcam preview using OpenCV.")
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

    window_name = "RoboLife Farm - OpenCV Camera Gray"
    print("Camera opened in grayscale mode. Press 'q' or Esc to exit.")

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("ERROR: Cannot read frame from camera")
                return 1

            gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            cv2.imshow(window_name, gray_frame)

            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), 27):
                break
    finally:
        cap.release()
        cv2.destroyAllWindows()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
