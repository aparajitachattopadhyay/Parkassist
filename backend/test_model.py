import cv2
import os
from ultralytics import YOLO

# ── Config ────────────────────────────────────────────────────────────────────
YOLO_MODEL     = "yolov8n.pt"
TARGET_CLASSES = [0, 1, 2, 3, 5, 15, 16]  # person, bicycle, car, motorcycle, bus, cat, dog

IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
VIDEO_EXTS = {".mp4", ".avi", ".mov", ".mkv"}

# ── Helpers ───────────────────────────────────────────────────────────────────

def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def process_image(image_path, model, out_dir):
    print(f"\n[IMG] {os.path.basename(image_path)}")
    img = cv2.imread(image_path)
    if img is None:
        print("  [!] Could not read — skipped.")
        return

    results = model.predict(source=img, classes=TARGET_CLASSES, conf=0.25, verbose=False)
    annotated = results[0].plot()

    # Save annotated copy
    out_path = os.path.join(out_dir, "detected_" + os.path.basename(image_path))
    cv2.imwrite(out_path, annotated)
    print(f"  [✓] Saved → {out_path}")

    # Show window
    cv2.imshow("ParkAssist — Image Detection (any key = next)", annotated)
    cv2.waitKey(0)
    cv2.destroyAllWindows()


def process_video(video_path, model, out_dir):
    print(f"\n[VDO] {os.path.basename(video_path)}")
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print("  [!] Could not open — skipped.")
        return

    fps    = cap.get(cv2.CAP_PROP_FPS) or 25
    width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    total  = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    out_name = "detected_" + os.path.splitext(os.path.basename(video_path))[0] + ".mp4"
    out_path = os.path.join(out_dir, out_name)
    writer   = cv2.VideoWriter(out_path, cv2.VideoWriter_fourcc(*"mp4v"), fps, (width, height))

    frame_idx = 0
    print(f"  Processing {total} frames at {fps:.1f} fps — press 'q' to skip to next file.")

    while cap.isOpened():
        success, frame = cap.read()
        if not success:
            break

        results         = model.predict(source=frame, classes=TARGET_CLASSES, conf=0.25, verbose=False)
        annotated_frame = results[0].plot()
        writer.write(annotated_frame)

        cv2.imshow("ParkAssist — Video Detection ('q' = skip)", annotated_frame)
        frame_idx += 1

        if cv2.waitKey(1) & 0xFF == ord("q"):
            print("  [→] Skipped by user.")
            break

    cap.release()
    writer.release()
    cv2.destroyAllWindows()
    print(f"  [✓] Saved → {out_path}")


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    base_dir  = os.path.dirname(os.path.abspath(__file__))
    media_dir = os.path.join(base_dir, "media")
    out_dir   = os.path.join(media_dir, "detected")
    ensure_dir(out_dir)

    # Collect all media files (skip the detected/ subfolder)
    all_files = sorted([
        f for f in os.listdir(media_dir)
        if os.path.isfile(os.path.join(media_dir, f))
        and os.path.splitext(f)[1].lower() in IMAGE_EXTS | VIDEO_EXTS
    ])

    if not all_files:
        print("[!] No images or videos found in", media_dir)
        exit()

    print(f"[*] Loading YOLO model: {YOLO_MODEL}")
    model = YOLO(YOLO_MODEL)
    print(f"[*] Found {len(all_files)} file(s) in media/ — processing one by one...\n")

    for filename in all_files:
        filepath = os.path.join(media_dir, filename)
        ext      = os.path.splitext(filename)[1].lower()

        if ext in IMAGE_EXTS:
            process_image(filepath, model, out_dir)
        elif ext in VIDEO_EXTS:
            process_video(filepath, model, out_dir)

        print("-" * 50)

    print(f"\n[✓] All done! Annotated files saved in: {out_dir}")
