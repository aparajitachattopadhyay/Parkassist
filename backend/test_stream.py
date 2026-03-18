"""
ParkAssist test streaming backend.

Reads a local video file, runs YOLO object detection, and provides:
1. An MJPEG stream of the video frames (original or annotated) on port 5000 (simulating ESP32-CAM).
2. A WebSocket server on port 8765 broadcasting bounding boxes to the frontend.
3. A /config endpoint to tell the frontend where the WS server is.
"""
import asyncio
import json
import time
import os
import cv2
from ultralytics import YOLO
from aiohttp import web
import websockets

import config

YOLO_MODEL = "yolov8n.pt"
TARGET_CLASSES = [0, 1, 2, 3, 5, 15, 16]

# Try to find a video in the media folder
base_dir = os.path.dirname(os.path.abspath(__file__))
video_path = os.path.join(base_dir, "media", "vdo.mp4")

if not os.path.exists(video_path):
    print(f"Error: Video file not found: {video_path}")
    exit(1)

model = YOLO(YOLO_MODEL)
latest_detections = []
ws_clients = set()
current_frame_jpg = None

def run_inference(frame):
    global latest_detections
    h, w = frame.shape[:2]
    results = model.predict(frame, verbose=False, classes=TARGET_CLASSES)
    out = []
    for r in results:
        if r.boxes is None:
            continue
        for box in r.boxes:
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            x1, y1, x2, y2 = box.xyxy[0].tolist()
            out.append({
                "label": config.COCO_ID_TO_NAME.get(cls_id, "object"),
                "confidence": conf,
                "x": x1 / w,
                "y": y1 / h,
                "w": (x2 - x1) / w,
                "h": (y2 - y1) / h,
            })
    return out

async def video_reader_loop():
    global current_frame_jpg, latest_detections
    print(f"Playing video: {video_path}")
    cap = cv2.VideoCapture(video_path)
    fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
    delay = 1.0 / fps

    frame_count = 0
    while True:
        start_t = time.time()
        ret, frame = cap.read()
        if not ret:
            # loop video
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            continue
            
        frame_count += 1
        
        # Run inference every N frames to save CPU
        if frame_count % config.INFERENCE_INTERVAL == 0:
            latest_detections = run_inference(frame)
        
        # Encode for MJPEG stream
        # Optional: resize to make streaming faster
        frame_resized = cv2.resize(frame, (640, 480))
        ret, buffer = cv2.imencode('.jpg', frame_resized, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        if ret:
            current_frame_jpg = buffer.tobytes()

        # Try to maintain original video FPS
        elapsed = time.time() - start_t
        if elapsed < delay:
            await asyncio.sleep(delay - elapsed)
        else:
            await asyncio.sleep(0.01)

# --- WebSocket Server (AI bounding boxes) ---
async def ws_handler(websocket):
    ws_clients.add(websocket)
    try:
        while True:
            await asyncio.sleep(0.15)
            if latest_detections:
                msg = json.dumps({"detections": latest_detections})
                await websocket.send(msg)
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        ws_clients.discard(websocket)

# --- HTTP Server (MJPEG Stream & Config) ---
async def mjpeg_handler(request):
    response = web.StreamResponse(
        status=200,
        reason='OK',
        headers={
            'Content-Type': 'multipart/x-mixed-replace; boundary=frame',
            'Connection': 'keep-alive',
            'Cache-Control': 'no-cache, no-store, must-revalidate',
            'Pragma': 'no-cache',
            'Expires': '0',
            'Access-Control-Allow-Origin': '*'
        }
    )
    await response.prepare(request)
    try:
        while True:
            if current_frame_jpg:
                await response.write(b'--frame\r\n')
                await response.write(b'Content-Type: image/jpeg\r\n\r\n')
                await response.write(current_frame_jpg)
                await response.write(b'\r\n')
            await asyncio.sleep(0.033) # max 30 Hz output
    except Exception:
        pass
    return response

async def config_handler(request):
    # Tell frontend our WS URL is running on localhost
    cfg = {
        "detectionWs": "ws://127.0.0.1:8765"
    }
    return web.json_response(cfg, headers={"Access-Control-Allow-Origin": "*"})

async def capture_handler(request):
    if current_frame_jpg:
        return web.Response(body=current_frame_jpg, content_type="image/jpeg", headers={"Access-Control-Allow-Origin": "*"})
    return web.Response(status=404, headers={"Access-Control-Allow-Origin": "*"})

async def start_http_server():
    app = web.Application()
    app.router.add_get('/stream', mjpeg_handler)
    app.router.add_get('/config', config_handler)
    app.router.add_get('/capture', capture_handler)

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', 5000)
    await site.start()
    print("HTTP Server started on http://127.0.0.1:5000")
    print("  - MJPEG Stream: http://127.0.0.1:5000/stream")
    print("  - Config Endpoint: http://127.0.0.1:5000/config")

async def main():
    print("Starting ParkAssist Test Streamer...")
    
    # Websocket server
    start_server = websockets.serve(ws_handler, "0.0.0.0", 8765)
    await start_server
    print("WebSocket detections running on ws://127.0.0.1:8765")
    
    # HTTP MJPEG server
    await start_http_server()
    
    # Video reader loop runs concurrently
    await video_reader_loop()

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopped.")
