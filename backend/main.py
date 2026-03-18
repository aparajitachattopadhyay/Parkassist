"""
ParkAssist backend: pull MJPEG from ESP32-CAM, run YOLO detection,
broadcast detections to connected clients via WebSocket.
Optionally register with ESP32 so the dashboard can auto-connect to this server.
"""
import asyncio
import json
import time
import cv2
import numpy as np
from ultralytics import YOLO

import config

# Global: latest detections for new clients
latest_detections = []
clients = set()
model = None
frame_count = 0


def get_detections(frame):
    global frame_count
    frame_count += 1
    if frame_count % config.INFERENCE_INTERVAL != 0:
        return latest_detections
    if model is None or frame is None:
        return []
    h, w = frame.shape[:2]
    results = model.predict(frame, verbose=False, classes=config.TARGET_CLASSES)
    out = []
    for r in results:
        if r.boxes is None:
            continue
        for box in r.boxes:
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            x1, y1, x2, y2 = box.xyxy[0].tolist()
            # Normalize to 0-1 for frontend overlay
            out.append({
                "label": config.COCO_ID_TO_NAME.get(cls_id, "object"),
                "confidence": conf,
                "x": x1 / w,
                "y": y1 / h,
                "w": (x2 - x1) / w,
                "h": (y2 - y1) / h,
            })
    return out


def _stream_reader_thread():
    """Blocking: pull MJPEG, run YOLO, update latest_detections."""
    global latest_detections
    cap = cv2.VideoCapture(config.STREAM_URL)
    if not cap.isOpened():
        print("Cannot open stream:", config.STREAM_URL)
        return
    print("Stream opened:", config.STREAM_URL)
    while True:
        ret, frame = cap.read()
        if not ret:
            time.sleep(0.5)
            continue
        detections = get_detections(frame)
        if detections:
            latest_detections = detections
        time.sleep(0.05)


async def broadcast_detections():
    """Periodically broadcast latest_detections to all WebSocket clients."""
    while True:
        await asyncio.sleep(0.15)
        if not latest_detections:
            continue
        msg = json.dumps({"detections": latest_detections})
        for ws in list(clients):
            try:
                await ws.send(msg)
            except Exception:
                pass


async def ws_handler(websocket, path):
    clients.add(websocket)
    try:
        if latest_detections:
            await websocket.send(json.dumps({"detections": latest_detections}))
        async for _ in websocket:
            pass
    finally:
        clients.discard(websocket)


def register_with_esp32():
    """Tell ESP32 our WebSocket URL so /config returns it to the dashboard."""
    if not config.BACKEND_HOST:
        return
    import urllib.request
    url = f"http://{config.ESP32_IP}/register"
    data = json.dumps({"host": config.BACKEND_HOST, "port": config.WS_PORT}).encode()
    try:
        req = urllib.request.Request(url, data=data, method="POST", headers={"Content-Type": "application/json"})
        urllib.request.urlopen(req, timeout=2)
        print("Registered with ESP32:", config.BACKEND_HOST, config.WS_PORT)
    except Exception as e:
        print("Register failed:", e)


async def main():
    global model
    import threading
    print("Loading YOLO", config.YOLO_MODEL)
    model = YOLO(config.YOLO_MODEL)
    register_with_esp32()
    threading.Thread(target=_stream_reader_thread, daemon=True).start()
    import websockets
    async with websockets.serve(ws_handler, config.WS_HOST, config.WS_PORT):
        print("WebSocket server on", config.WS_HOST + ":" + str(config.WS_PORT))
        asyncio.create_task(broadcast_detections())
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
