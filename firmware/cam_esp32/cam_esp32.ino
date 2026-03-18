/*
 * ParkAssist — Split Test: ESP32-CAM side
 * ----------------------------------------
 * Flash this to the ESP32-CAM (AI-Thinker module).
 *
 * What it does:
 *   - Joins the "ParkAssist" WiFi AP created by the sensor ESP32
 *   - Serves MJPEG camera stream at http://192.168.4.2/stream
 *   - Dashboard at 192.168.4.1 auto-connects to this stream
 *
 * Board: "AI Thinker ESP32-CAM" in Arduino IDE
 * NO wiring changes needed — camera is built-in on the module.
 *
 * To flash: Use a USB-TTL adapter
 *   TX  → pin UOR
 *   RX  → pin UOT
 *   GND → GND
 *   5V  → 5V
 *   IO0 → GND (only during flashing — remove after flash)
 */

#include "esp_camera.h"
#include "esp_http_server.h"
#include "WiFi.h"

// ─── Must join the AP created by the sensor ESP32 ────────────────────────────
#define WIFI_SSID  "ParkAssist"
#define WIFI_PASS  "parkassist"
// ─────────────────────────────────────────────────────────────────────────────

// AI-Thinker ESP32-CAM pin map
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static camera_config_t cam_cfg = {
  .pin_pwdn     = PWDN_GPIO_NUM,  .pin_reset  = RESET_GPIO_NUM,
  .pin_xclk     = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM, .pin_d6 = Y8_GPIO_NUM, .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM, .pin_d3 = Y5_GPIO_NUM, .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM, .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM, .pin_href = HREF_GPIO_NUM, .pin_pclk = PCLK_GPIO_NUM,
  .xclk_freq_hz  = 20000000,
  .ledc_timer    = LEDC_TIMER_0,
  .ledc_channel  = LEDC_CHANNEL_0,
  .pixel_format  = PIXFORMAT_JPEG,
  .frame_size    = FRAMESIZE_VGA,   // 640×480 — good balance of quality vs speed
  .jpeg_quality  = 14,              // 0 best, 63 worst
  .fb_count      = 2,
  .grab_mode     = CAMERA_GRAB_WHEN_EMPTY
};

// ─── MJPEG stream handler ─────────────────────────────────────────────────────
static esp_err_t streamHandler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  char part_buf[64];
  httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) break;
    snprintf(part_buf, 64, "Content-Length: %u\r\n\r\n", (uint32_t)fb->len);
    if (httpd_resp_send_chunk(req, "\r\n--frame\r\nContent-Type: image/jpeg\r\n", 37) != ESP_OK) break;
    if (httpd_resp_send_chunk(req, part_buf, strlen(part_buf)) != ESP_OK) break;
    if (httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len) != ESP_OK) break;
    esp_camera_fb_return(fb);
  }
  if (fb) esp_camera_fb_return(fb);
  return ESP_OK;
}

// ─── Root handler — redirect to sensor dashboard ──────────────────────────────
static esp_err_t rootHandler(httpd_req_t *req) {
  httpd_resp_set_hdr(req, "Location", "http://192.168.4.1");
  httpd_resp_set_status(req, "302 Found");
  return httpd_resp_send(req, NULL, 0);
}

static void startCamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = streamHandler };
    httpd_uri_t root_uri   = { .uri = "/",       .method = HTTP_GET, .handler = rootHandler   };
    httpd_register_uri_handler(server, &stream_uri);
    httpd_register_uri_handler(server, &root_uri);
    Serial.println("Camera HTTP server started");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ParkAssist CAM ESP32 ===");

  // Init camera
  if (esp_camera_init(&cam_cfg) != ESP_OK) {
    Serial.println("Camera init FAILED");
    return;
  }
  Serial.println("Camera OK");

  // Join sensor ESP32's AP — will get IP 192.168.4.2
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Joining '%s'", WIFI_SSID);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > 15000) {
      Serial.println("\nTimeout — could not join ParkAssist AP");
      return;
    }
    delay(400); Serial.print(".");
  }
  Serial.printf("\nCam IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("Stream: http://" + WiFi.localIP().toString() + "/stream");

  startCamServer();
}

void loop() {
  delay(1000); // nothing to do — camera server runs on its own task
}
