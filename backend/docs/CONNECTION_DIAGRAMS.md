# ParkAssist – Connection & Wiring Diagrams

## 1. System overview (data flow)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           PARKASSIST SYSTEM                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────┐     WiFi AP "ParkAssist"      ┌──────────────────────┐  │
│   │  ESP32-CAM   │◄──────────────────────────────►│  Phone (dashboard)    │  │
│   │ 192.168.4.1  │     • Open http://192.168.4.1  │  • No IP entry       │  │
│   │              │     • Stream + sensors + page  │  • Landscape UI       │  │
│   └──────┬───────┘                               └──────────┬───────────┘  │
│          │                                                  │              │
│          │ MJPEG stream                                      │ WS detections│
│          ▼                                                  │ (optional)   │
│   ┌──────────────┐     same WiFi "ParkAssist"     ┌──────────▼───────────┐  │
│   │   Backend    │◄──────────────────────────────►│  Phone (dashboard)   │  │
│   │  (laptop)    │     • Backend joins ParkAssist  │  • /config gives     │  │
│   │ 192.168.4.2  │     • Registers WS URL         │    detectionWs URL  │  │
│   │  YOLO + WS   │     • Pulls /stream, runs AI    │  • Connects to WS    │  │
│   └──────────────┘                               └──────────────────────┘  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2. ESP32-CAM wiring (hardware)

**Assumption:** No SD card used, so GPIO 12, 13, 14, 15 are free for the two ultrasonic sensors.

```
                    ESP32-CAM (AI-Thinker)
                    ┌─────────────────────┐
                    │  [CAMERA MODULE]    │
                    │                     │
        5V ─────────┤ 5V        GND   ◄───┼─── GND
                    │                     │
                    │  GPIO 12 ────────────┼──────► HC-SR04 #1 TRIG  (Sensor A)
                    │  GPIO 13 ───────────┼──────► HC-SR04 #1 ECHO  (Sensor A)
                    │  GPIO 14 ───────────┼──────► HC-SR04 #2 TRIG  (Sensor B)
                    │  GPIO 15 ───────────┼──────► HC-SR04 #2 ECHO  (Sensor B)
                    │                     │
                    │  GPIO 1 (TX) ───────┼──────► BOTH LiDARs SCL (I2C)
                    │  GPIO 3 (RX) ───────┼──────► BOTH LiDARs SDA (I2C)
                    │  GPIO 4     ────────┼──────► LiDAR #2 XSHUT (For Address change)
                    │                     │
                    │  (Camera uses 0,2,4,5,18,19,  │
                    │   21,22,23,25,26,27,32,34,35, │
                    │   36,39 – do not use these)   │
                    └─────────────────────┘

     HC-SR04 #1 (Left)              HC-SR04 #2 (Right)
     ┌─────────────┐                ┌─────────────┐
     │ VCC  TRIG   │                │ VCC  TRIG   │
     │  │    │     │                │  │    │     │
     │  5V   GPIO12│                │  5V   GPIO14│
     │ ECHO  GND   │                │ ECHO  GND   │
     │  │     │    │                │  │     │    │
     │ GPIO13 GND  │                │ GPIO15 GND  │
     └─────────────┘                └─────────────┘
```

**Power:** ESP32-CAM can draw ~300–500 mA. Use a stable 5 V supply (e.g. USB or regulator from Li-ion). Do not power only from a small solar panel without a battery buffer.

## 3. Optional: Solar + Li-ion (block diagram)

```
     ┌─────────────┐      ┌─────────────────┐      ┌─────────────┐
     │ Solar panel │─────►│ Charging module │─────►│ Li-ion      │
     │ (6V / 1W+)  │      │ (e.g. TP4056)   │      │ 1S 3.7V     │
     └─────────────┘      └────────┬────────┘      └──────┬──────┘
                                   │                      │
                                   │                      │ 3.3V or 5V
                                   │                      │ step-down
                                   │                      ▼
                                   │               ┌─────────────┐
                                   └──────────────►│  ESP32-CAM  │
                                      (optional    │  + sensors  │
                                       battery     └─────────────┘
                                       protection)
```

- Solar → TP4056 (or similar) → Li-ion. Output of battery through a 3.3 V or 5 V step-down to ESP32-CAM and sensors.
- No software change; this is power only.

## 4. Phone connection (zero config)

1. Power on ESP32-CAM.
2. On the phone, open **Wi‑Fi settings** and connect to **ParkAssist** (password: `parkassist`).
3. Open the browser and go to: **http://192.168.4.1**  
   (No need to type the ESP32’s IP elsewhere – the page is served from the device, so the app uses the same host.)
4. Rotate to **landscape**; the dashboard loads (camera, sensors, top view).  
No “Change IP” or manual IP entry in normal use.

## 5. Optional: Backend (AI detection) connection

1. Connect the **laptop** to the same Wi‑Fi **ParkAssist** (it will get an IP like 192.168.4.2).
2. Set the backend to use that IP when registering:
   - `PARKASSIST_BACKEND_HOST=192.168.4.2` (or your laptop’s IP on ParkAssist).
3. Run the backend (it pulls the stream from 192.168.4.1 and registers its WebSocket URL with the ESP32).
4. The dashboard fetches **http://192.168.4.1/config**; the ESP32 returns `detectionWs: "ws://192.168.4.2:8765"`.
5. The phone connects to that WebSocket and shows AI bounding boxes without any IP input from the user.

## 6. Pin summary (ESP32-CAM, firmware default)


| GPIO | Function        | Note                    |
|------|-----------------|-------------------------|
| 12   | Sensor A TRIG   | HC-SR04 left            |
| 13   | Sensor A ECHO   | HC-SR04 left            |
| 14   | Sensor B TRIG   | HC-SR04 right           |
| 15   | Sensor B ECHO   | HC-SR04 right           |
| 1    | LiDAR I2C TX/SCL| I2C SCL for LiDARs      |
| 3    | LiDAR I2C RX/SDA| I2C SDA for LiDARs      |
| 2,4,5,18,19,21,22,23,25,26,27,32,34,35,36,39 | Camera | Do not use for sensors |

**Note on LiDAR ToF Sensors (I2C):**
The ESP32-CAM is extremely limited on available GPIO pins because the camera uses almost all of them. 
To connect the 2 new LiDAR ToF sensors (like VL53L1X or VL53L0X), you must put them on a shared **I2C Bus**.
Since all standard GPIOs are taken, you will need to re-purpose the **Serial Hardware TX/RX pins (GPIO 1 and 3)** to act as your I2C SDA and SCL pins after the board has booted.

**Wiring the LiDARs:**
- **LiDAR 1 (Left) & LiDAR 2 (Right) VIN:** 3.3V or 5V (Check your specific sensor module)
- **LiDAR 1 & 2 GND:** GND
- **LiDAR 1 & 2 SCL:** Connect BOTH to ESP32-CAM **GPIO 1 (U0TXD)**
- **LiDAR 1 & 2 SDA:** Connect BOTH to ESP32-CAM **GPIO 3 (U0RXD)**
- **LiDAR 2 XSHUT (Important):** Connect to **GPIO 2** or **GPIO 4**. Because both LiDARs have the same default I2C address (usually 0x29), you must wire the XSHUT pin of one LiDAR to a free pin (like GPIO 4) to keep it turned off during boot, change the address of the first LiDAR via software, and then turn the second one on.
