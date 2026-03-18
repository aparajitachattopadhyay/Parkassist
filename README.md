
# ParkAssist: AI-Assisted Parking & Obstacle Awareness

Team Binary - **Aparajita Chattopadhyay (Lead)**, **Ankan Bhowmik**

ParkAssist is a low-cost, embedded + web dashboard system that helps with **parking assistance** and **near-field obstacle awareness** using:
- **Ultrasonic sensors** (left/right distance + fused distance/angle)
- **ESP32-CAM video stream** (MJPEG)
- Optional **AI object detection** (YOLOv8) running on a laptop/PC, streamed to the dashboard via WebSocket

Live deployment: https://parkassist-plum.vercel.app/

The dashboard runs in a phone browser (landscape UI). In normal hardware mode, **no manual IP input is needed** - you simply connect to the device Wi-Fi and open the provided URL.

---

## Demo screenshots / media

- Dashboard UI: `gallery/frontend.png`
- Hardware photos: `gallery/hardware.jpeg`, `gallery/hardware2.jpeg`
- - YouTube demo video: https://youtu.be/RprVM_lP-LA

---

## Repository structure

- `frontend/`: Static dashboard (`index.html`, `script.js`, `styles.css`)
- `backend/`: Python backend for YOLO detection and streaming tests
- `firmware/`: Arduino sketches for sensor ESP32 and ESP32-CAM
- `docs/`: Wiring/connection diagrams and documentation
- `testing/`: Small testing artifacts/utilities

---

## System architecture (recommended "split" setup)

This repo implements a **split architecture**:
- **Sensor ESP32** creates Wi-Fi AP `ParkAssist` and serves:
  - Dashboard web page on port **80**
  - Sensor WebSocket on port **81**
  - Optional `/config` endpoint (used by the dashboard to auto-connect to AI detection WS)
- **ESP32-CAM** joins that Wi-Fi and provides:
  - MJPEG camera stream at `http://192.168.4.2/stream`
- **Optional laptop/PC backend** joins the same Wi-Fi and provides:
  - YOLO detections WebSocket (default) on port **8765**

Wiring + connection diagrams live in `docs/CONNECTION_DIAGRAMS.md`.

### Default device addresses/ports (split setup)

- **Sensor ESP32**: `192.168.4.1`
  - Dashboard: `http://192.168.4.1`
  - Sensor WS: `ws://192.168.4.1:81`
  - Config endpoint: `http://192.168.4.1/config`
- **ESP32-CAM**: `192.168.4.2`
  - MJPEG stream: `http://192.168.4.2/stream`
- **AI backend (laptop/PC)**:
  - Detections WS (default): `ws://<BACKEND_HOST>:8765`

---

## Quick start (hardware)

### 1) Flash firmware

#### A) Sensor ESP32 (plain ESP32, not ESP32-CAM)

- Sketch: `firmware/sensor_esp32/sensor_esp32.ino`
- Creates Wi-Fi AP:
  - **SSID**: `ParkAssist`
  - **Password**: `parkassist`

Arduino libraries (via Library Manager):
- **WebSockets** by Markus Sattler
- **ArduinoJson** by Benoit Blanchon

Ultrasonic wiring (as used by the sketch):
- HC-SR04 **Left**: TRIG -> GPIO 12, ECHO -> GPIO 13
- HC-SR04 **Right**: TRIG -> GPIO 14, ECHO -> GPIO 15

Important electrical note:
- HC-SR04 **ECHO is 5V**, ESP32 GPIO is **3.3V**.
- Use a voltage divider per ECHO line (example is included in the sketch header).

#### B) ESP32-CAM (AI Thinker module)

- Sketch: `firmware/cam_esp32/cam_esp32.ino`
- Joins the sensor AP and starts MJPEG streaming at `http://192.168.4.2/stream`.

Flashing note: ESP32-CAM typically requires a USB-TTL adapter and IO0 to GND during flashing (details in the sketch header).

### 2) Power-on order

1. Power **Sensor ESP32** (starts Wi-Fi AP `ParkAssist`)
2. Power **ESP32-CAM** (joins AP, becomes `192.168.4.2`)

### 3) Connect and open the dashboard (phone)

1. On your phone, connect Wi-Fi to **ParkAssist** (password: `parkassist`)
2. Open browser -> `http://192.168.4.1`
3. Rotate to **landscape** for the full UI

---

## Optional: AI detection backend (YOLOv8 on laptop/PC)

The backend pulls the ESP32-CAM MJPEG stream and broadcasts **AI bounding boxes** to the dashboard over WebSocket.

### Prerequisites

- Python 3.9+ recommended
- A machine on the same Wi-Fi network as the ESP32 devices

### Setup (Windows PowerShell)

From repo root:

```powershell
cd "backend"
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

### Run the backend

Set `PARKASSIST_BACKEND_HOST` to your laptop/PC IP on the **ParkAssist** Wi-Fi so the sensor ESP32 can tell the phone where to connect.

Example (replace with your IP):

```powershell
$env:PARKASSIST_BACKEND_HOST="192.168.4.2"
python main.py
```

Useful environment variables (see `backend/config.py`):

- `PARKASSIST_SENSOR_IP`: sensor ESP32 IP (default `192.168.4.1`)
- `PARKASSIST_CAM_IP`: ESP32-CAM IP (default `192.168.4.2`)
- `PARKASSIST_BACKEND_HOST`: this machine's IP (default `None` -> disables registration)
- `PARKASSIST_WS_HOST`: bind host (default `0.0.0.0`)
- `PARKASSIST_WS_PORT`: detection WS port (default `8765`)
- `PARKASSIST_YOLO_MODEL`: model path/name (default `yolov8n.pt`)
- `PARKASSIST_INFERENCE_INTERVAL`: inference every N frames (default `3`)

### What "registration" means

If `PARKASSIST_BACKEND_HOST` is set, the backend calls `http://192.168.4.1/register` so the sensor ESP32 can return a `detectionWs` URL from `http://192.168.4.1/config`.

The dashboard then auto-connects to AI without manual configuration.

---

## Local demo / testing (no ESP32 required)

This repo includes a test streamer that simulates an ESP32-CAM stream using a local video file and serves `/config` for the dashboard.

### 1) Install backend deps

```powershell
cd "backend"
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
pip install aiohttp
```

Note: `aiohttp` is used by `backend/test_stream.py`.

### 2) Ensure a demo video exists

`backend/test_stream.py` expects `backend/media/vdo.mp4`.

### 3) Run the local test streamer

```powershell
cd "backend"
.\.venv\Scripts\Activate.ps1
python test_stream.py
```

It starts:
- MJPEG stream: `http://127.0.0.1:5000/stream`
- Config: `http://127.0.0.1:5000/config`
- AI detections WS: `ws://127.0.0.1:8765`

### 4) Open the dashboard locally

Open `frontend/index.html` in a browser (double-click or "Open with").

The dashboard detects `localhost/127.0.0.1` or `file://` and automatically switches to the local test endpoints (see `isLocal` logic in `frontend/script.js`).

---

## Troubleshooting

- **Dashboard doesn't load**
  - Ensure you're connected to Wi-Fi **ParkAssist**
  - Open exactly `http://192.168.4.1` (sensor ESP32)

- **Camera shows "Connecting..."**
  - Confirm ESP32-CAM is powered and joined the AP
  - The stream URL must be reachable: `http://192.168.4.2/stream`

- **No sensor values**
  - Check Sensor ESP32 WebSocket is reachable: `ws://192.168.4.1:81`
  - Verify TRIG/ECHO wiring and voltage dividers on ECHO lines

- **AI pill stays off**
  - Backend must be running and reachable from the phone
  - Set `PARKASSIST_BACKEND_HOST` correctly (your laptop/PC IP on ParkAssist Wi-Fi)
  - If you're demoing locally, use `python test_stream.py` and open `frontend/index.html` locally

- **High CPU usage on backend**
  - Increase `PARKASSIST_INFERENCE_INTERVAL`
  - Use a smaller model (`yolov8n.pt`) or reduce camera frame size on the ESP32-CAM

---

## Credits

### Contributors

- **Team Binary**
  - **Aparajita Chattopadhyay (Lead)**
  - **Ankan Bhowmik**

### Libraries / frameworks

- **Ultralytics YOLOv8** (Python)
- **OpenCV** (Python)
- **ArduinoJson** (firmware)
- **WebSockets (Markus Sattler)** (firmware)

### YouTube (testing / learning references)

Paul McWhorter - "ESP32-CAM Video Streaming Tutorial"
https://www.youtube.com/watch?v=visj0KE5VtY

DroneBot Workshop - "ESP32 Ultrasonic Distance Sensor HC-SR04 Tutorial"
https://www.youtube.com/watch?v=4JdRr2oEJpA

Ultralytics - "YOLOv8 Official Introduction & Usage Guide"
https://www.youtube.com/watch?v=Z-65nqxUdl4

Note: These credits are for **testing/learning reference** only; ParkAssist is an original student project implementation.
