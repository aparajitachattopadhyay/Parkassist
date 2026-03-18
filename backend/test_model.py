import cv2
import os
from ultralytics import YOLO

# Configuration
YOLO_MODEL = "yolov8n.pt"
TARGET_CLASSES = [0, 1, 2, 3, 5, 15, 16]  # person, bicycle, car, motorcycle, bus, cat, dog

def test_on_image(image_path, model):
    if not os.path.exists(image_path):
        print(f"[!] Image not found: {image_path}")
        return
    print(f"[*] Running object detection on image: {image_path}")

    img = cv2.imread(image_path)

    results = model.predict(
        source=img,
        classes=TARGET_CLASSES,
        conf=0.25
    )

    annotated_img = results[0].plot()

    print(">>> Image window opened! Press any key in the image window to close it and continue. <<<")
    cv2.imshow("ParkAssist - Image Test", annotated_img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()


def test_on_video(video_path, model):
    if not os.path.exists(video_path):
        print(f"[!] Video not found: {video_path}")
        return
    print(f"[*] Running object detection on video: {video_path}")

    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"[!] Could not open video: {video_path}")
        return

    print(">>> Video window opened! Press 'q' in the video window to stop playback. <<<")
    while cap.isOpened():
        success, frame = cap.read()
        if not success:
            print("Playback finished.")
            break

        results = model.predict(
            source=frame,
            classes=TARGET_CLASSES,
            conf=0.25,
            verbose=False
        )

        annotated_frame = results[0].plot()
        cv2.imshow("ParkAssist - Video Test", annotated_frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("Playback stopped by user.")
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    print(f"[*] Loading YOLO model: {YOLO_MODEL}")
    model = YOLO(YOLO_MODEL)

    base_dir = os.path.dirname(os.path.abspath(__file__))
    img_path = os.path.join(base_dir, "media", "img.jpeg")
    vdo_path = os.path.join(base_dir, "media", "vdo.mp4")

    test_on_image(img_path, model)
    print("-" * 40)
    test_on_video(vdo_path, model)
