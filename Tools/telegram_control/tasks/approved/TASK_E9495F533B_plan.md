# TASK_E9495F533B - Display OpenCV camera preview in grayscale

## Request
sửa code opencv_camera.py hiển thị ảnh gray cho tôi

## Plan
1. Update Project_Farm_Here/opencv_camera.py to convert each captured BGR frame to grayscale using cv2.cvtColor.
2. Show the grayscale frame in the OpenCV preview window instead of the original color frame.
3. Keep existing camera arguments and quit controls unchanged.

## Files
- `Project_Farm_Here/opencv_camera.py`: Convert each camera frame from BGR to grayscale and display the grayscale image.

## Summary
opencv_camera.py will read frames from the camera, convert them to gray with cv2.COLOR_BGR2GRAY, and display the grayscale image in the preview window.
