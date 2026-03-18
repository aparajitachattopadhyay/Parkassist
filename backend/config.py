"""
ParkAssist backend config.
When your laptop/PC is connected to the ESP32's WiFi "ParkAssist":
  - Sensor ESP32  →  192.168.4.1  (WebSocket :81, serves the frontend)
  - ESP32-CAM     →  192.168.4.2  (MJPEG stream at /stream)

Set BACKEND_HOST to this machine's IP on that network so the dashboard
can auto-connect to the AI WebSocket (or leave None to skip registration).
"""
import os

# ── ESP32 addresses ────────────────────────────────────────────────────────────
# Sensor ESP32 (serves the dashboard + WebSocket sensor data)
ESP32_SENSOR_IP = os.environ.get("PARKASSIST_SENSOR_IP", "192.168.4.1")

# ESP32-CAM (MJPEG video stream) — always 192.168.4.2 in split architecture
ESP32_CAM_IP    = os.environ.get("PARKASSIST_CAM_IP",    "192.168.4.2")
STREAM_URL      = f"http://{ESP32_CAM_IP}/stream"

# Keep ESP32_IP pointing at sensor for /register endpoint
ESP32_IP = ESP32_SENSOR_IP

# ── Media dir (for test_model.py) ──────────────────────────────────────────────
MEDIA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "media")

# WebSocket server for detection updates (this backend)
WS_HOST = os.environ.get("PARKASSIST_WS_HOST", "0.0.0.0")
WS_PORT = int(os.environ.get("PARKASSIST_WS_PORT", "8765"))

# For ESP32 /config: URL that the phone can use to connect to this backend.
# Set to your machine's IP on the ParkAssist WiFi, or leave None to skip registration.
BACKEND_HOST = os.environ.get("PARKASSIST_BACKEND_HOST", None)

# YOLO model: "yolov8n" (fast) or "yolov8s" (more accurate)
YOLO_MODEL = os.environ.get("PARKASSIST_YOLO_MODEL", "yolov8n.pt")

# Inference every N frames to reduce CPU load
INFERENCE_INTERVAL = int(os.environ.get("PARKASSIST_INFERENCE_INTERVAL", "3"))

# COCO class IDs we care about (Indian streets: person, car, bus, bicycle, motorcycle, cat, dog)
TARGET_CLASSES = [0, 1, 2, 3, 5, 15, 16]  # person, bicycle, car, motorcycle, bus, cat, dog
COCO_ID_TO_NAME = {0: "person", 1: "bicycle", 2: "car", 3: "motorcycle", 5: "bus", 15: "cat", 16: "dog"}
