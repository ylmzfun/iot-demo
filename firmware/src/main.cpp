// 精简版 ESP32-CAM CameraWebServer（AI-Thinker ESP32-CAM）
//
// 提供两个 HTTP 服务，和官方示例布局一致：
//   - 端口 80  /          一个简单信息页（含画面预览）
//   - 端口 80  /capture   抓取一张 jpg
//   - 端口 81  /stream    multipart/x-mixed-replace 的 MJPEG 视频流
//
// 配合本仓库的 Python 客户端：uv run main.py --url <设备IP>
// （客户端默认连 http://<ip>:81/stream）

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include "secrets.h"

// ---------- AI-Thinker ESP32-CAM 摄像头引脚 ----------
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static httpd_handle_t camera_httpd = NULL;  // 端口 80
static httpd_handle_t stream_httpd = NULL;  // 端口 81

// ---------- /stream ：MJPEG 视频流 ----------
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t jpg_len = 0;
  uint8_t *jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("摄像头取帧失败");
      res = ESP_FAIL;
    } else if (fb->format != PIXFORMAT_JPEG) {
      bool ok = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!ok) res = ESP_FAIL;
    } else {
      jpg_len = fb->len;
      jpg_buf = fb->buf;
    }

    if (res == ESP_OK)
      res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) {
      size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, jpg_len);
      res = httpd_resp_send_chunk(req, part_buf, hlen);
    }
    if (res == ESP_OK)
      res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      jpg_buf = NULL;
    } else if (jpg_buf) {
      free(jpg_buf);
      jpg_buf = NULL;
    }
    if (res != ESP_OK) break;  // 客户端断开
  }
  return res;
}

// ---------- /capture ：抓一张 jpg ----------
static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

// ---------- / ：信息页 ----------
static esp_err_t index_handler(httpd_req_t *req) {
  char page[512];
  String ip = WiFi.localIP().toString();
  snprintf(page, sizeof(page),
           "<!doctype html><html><head><meta charset='utf-8'>"
           "<title>ESP32-CAM</title></head>"
           "<body style='text-align:center;font-family:sans-serif'>"
           "<h2>ESP32-CAM (AI-Thinker)</h2>"
           "<p>stream: http://%s:81/stream</p>"
           "<img src='http://%s:81/stream' style='max-width:100%%'/>"
           "</body></html>",
           ip.c_str(), ip.c_str());
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, page, strlen(page));
}

static void start_servers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;

  httpd_uri_t index_uri = {"/", HTTP_GET, index_handler, NULL};
  httpd_uri_t capture_uri = {"/capture", HTTP_GET, capture_handler, NULL};
  httpd_uri_t stream_uri = {"/stream", HTTP_GET, stream_handler, NULL};

  // 端口 80：信息页 + 抓拍
  config.server_port = 80;
  config.ctrl_port = 32080;
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
  }

  // 端口 81：视频流（单独服务，避免长连接占满）
  config.server_port = 81;
  config.ctrl_port = 32081;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_SVGA;  // 800x600
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;  // 数字越小画质越好、越占带宽
  config.fb_count = 2;

  if (!psramFound()) {
    Serial.println("未检测到 PSRAM，降级配置");
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("摄像头初始化失败 0x%x，请检查排线/板型/供电\n", err);
    delay(3000);
    ESP.restart();
  }
  Serial.println("摄像头初始化成功");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);
  Serial.printf("连接 WiFi: %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  Serial.print("Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");

  start_servers();
}

void loop() {
  delay(10000);
}
