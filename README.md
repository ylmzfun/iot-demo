# ESP32-CAM 摄像头实时预览 + YOLO 物体识别

自己烧录精简版 CameraWebServer 固件，让 ESP32-CAM 通过 WiFi 提供 MJPEG 视频流，
再用电脑上的 Python (OpenCV) 实时预览，并叠加 YOLO 目标检测（框 + 类别 + 置信度）。

```
ESP32-CAM 固件(firmware/)  ──WiFi MJPEG 流──►  本机 Python 客户端(main.py)
                                                  ├─ OpenCV 实时预览
                                                  └─ YOLO 物体识别(ultralytics)
```

本仓库包含**两端**：

- `firmware/` —— 烧进 ESP32-CAM 的固件（PlatformIO 工程，C/C++）
- `main.py` / `esp32cam.py` —— 跑在电脑上的 Python 客户端

## 硬件

- 板子：**AI-Thinker ESP32-CAM**（模组丝印 `ESP-32S`，FCC ID `2BCLP-ESP-32S`），
  本机实测是**不带 PSRAM** 的 WROOM 版本，固件已自动降级处理。
- 摄像头：OV2640。
- 烧录：板子本身无 USB，插在 **ESP32-CAM-MB 底板（CH340）** 上，
  通过 USB 连到电脑（macOS 上是 `/dev/cu.usbserial-210`）。
- WiFi：ESP32 只支持 **2.4GHz**，必须和电脑在同一局域网。

---

## 一、烧录固件（firmware/）

固件是一份精简版 CameraWebServer，提供：

| 地址 | 说明 |
| --- | --- |
| `http://<ip>/` | 信息页（含画面预览） |
| `http://<ip>/capture` | 抓取一张 jpg |
| `http://<ip>:81/stream` | MJPEG 视频流（Python 客户端连这个） |

### 1. 填 WiFi

编辑 [firmware/src/secrets.h](firmware/src/secrets.h)，改成你的 2.4G WiFi：

```c
#define WIFI_SSID     "你的WiFi名"
#define WIFI_PASSWORD "你的WiFi密码"
```

> 该文件已在 `.gitignore` 里，不会被提交。

### 2. 编译并烧录

用 PlatformIO（本机的 Homebrew `pio` 解释器损坏，用 `uvx` 起一个干净的）：

```bash
cd firmware
uvx --from platformio pio run -t upload
```

`platformio.ini` 里已配好板型 `esp32cam`、串口 `/dev/cu.usbserial-210`。
若你的串口名不同，改 `upload_port` / `monitor_port`。

### 3. 看串口日志拿 IP

```bash
cd firmware
uvx --from platformio pio device monitor
```

正常应看到：

```
摄像头初始化成功
连接 WiFi: 你的WiFi名 ....
Camera Ready! Use 'http://192.168.0.106' to connect
Stream: http://192.168.0.106:81/stream
```

记下这个 IP，下一步要用。

---

## 二、运行 Python 客户端

客户端用 `requests` 拉取 MJPEG 流，手动按 JPEG 起止标记（`FFD8`/`FFD9`）切帧，
再用 OpenCV 解码显示。手动解析不依赖 OpenCV 的 FFMPEG 后端，对 ESP32-CAM 这种
简单 MJPEG 流更稳定、延迟更低，断线还能自动重连。

### 安装依赖

```bash
uv sync
```

### 启动预览

把 IP 换成上一步拿到的（默认就开启 YOLO 识别）：

```bash
uv run main.py --url 192.168.0.106             # 预览 + YOLO 识别
uv run main.py --url 192.168.0.106 --no-detect # 只预览，不识别
```

也可以用环境变量：

```bash
ESP32_CAM_URL=192.168.0.106 uv run main.py
```

### YOLO 物体识别

- 主机端用 [ultralytics](https://docs.ultralytics.com/) 跑 YOLOv8，默认权重 `yolov8n.pt`
  （nano，最快），**首次运行自动下载**。Apple Silicon 自动走 MPS 加速。
- 识别到的目标会画框并标注类别 + 置信度（COCO 80 类）。
- 相关参数：

  | 参数 | 默认 | 说明 |
  | --- | --- | --- |
  | `--detect` / `--no-detect` | 开 | 是否开启识别 |
  | `--model` | `yolov8n.pt` | 权重；`yolov8s/m/l/x.pt` 越大越准越慢 |
  | `--conf` | `0.4` | 置信度阈值，越高框越少越准 |

> **性能说明**：实测 YOLOv8n 在 MPS 上推理仅 ~11ms/帧（~95 FPS），几乎不耗时。
> 真正的帧率瓶颈是 **WiFi 传输**：本机在共享网络下 SVGA / VGA 实测都只有 ~1–1.5 FPS，
> 降分辨率并不能提升帧率（说明瓶颈是网络而非摄像头编码）。要更流畅，优先改善 WiFi
> 信号：把 ESP32 挪近路由器、换不拥堵的 2.4G 信道、检查板上 PCB/u.FL 天线选择焊点。

### 窗口快捷键

| 按键 | 作用 |
| --- | --- |
| `q` / `Esc` | 退出 |
| `s` | 保存当前帧为 `snapshot_<时间戳>.jpg` |
| `d` | 开 / 关 YOLO 识别 |

窗口左上角叠加显示实时 FPS、分辨率和当前目标数（`YOLO:N`）。

---

## 项目结构

```
esp-32s/
├── main.py                  # 客户端入口：预览窗口、YOLO 叠加、FPS、截图、退出
├── esp32cam.py              # MJPEG 流读取与逐帧解码（含自动重连）
├── detector.py              # YOLO 目标检测封装（ultralytics）
├── pyproject.toml           # Python 依赖：opencv-python, requests, ultralytics
├── firmware/                # ESP32-CAM 固件（PlatformIO）
│   ├── platformio.ini       # 板型/串口/分区配置
│   └── src/
│       ├── main.cpp         # 精简版 CameraWebServer（AI-Thinker 引脚）
│       └── secrets.h        # WiFi 账号密码（不入库）
└── README.md
```

---

## 常见问题

- **串口日志报 `Camera probe failed 0x105 (ESP_ERR_NOT_FOUND)`**
  - 摄像头排线没插好（最常见）。**断电**后翻开 FFC 座的锁扣，把排线插到底
    （金手指那面朝下、朝板子），压紧锁扣，再上电。
  - 若仍不行，检查排线有无折损、是否真是 AI-Thinker 板型（其它板型要改
    `firmware/src/main.cpp` 顶部的引脚宏）。

- **`PSRAM ID read error` / 未检测到 PSRAM**
  - 说明是不带 PSRAM 的 ESP-32S(WROOM) 版本，固件已自动降级（DRAM、单缓冲、SVGA），
    不影响出图，只是分辨率/帧率上限更低。

- **设备能 ping 通但端口全 closed / 没服务**
  - 多半是摄像头初始化失败导致固件重启循环（WiFi 那步没执行）。看串口日志定位，
    优先排查上面的 `0x105`。

- **Python 端一直「重连中」**
  - 确认电脑和 ESP32 在同一局域网，先用浏览器开 `http://<ip>` 验证。
  - IP 可能因 DHCP 变化，以串口日志里最新的为准。

- **画面卡 / FPS 低**
  - 实测瓶颈是 **WiFi 传输**而非分辨率（SVGA/VGA 帧率差不多，都 ~1–1.5 FPS）。
    优先改善信号：ESP32 挪近路由器、换不拥堵的 2.4G 信道、检查天线选择焊点。
  - YOLO 本身在 MPS 上只占 ~11ms，不是瓶颈；按 `d` 关掉识别帧率也不会明显变高。
  - 别同时用浏览器和 Python 拉同一路流，会更卡。

---

## 后续可扩展

Python 端聚焦「实时预览」。`esp32cam.MJPEGCamera.frames()` 是一个逐帧产出 BGR
图像的生成器，可以很方便地在它之上接：录像、运动检测、Web 仪表盘、模型推理等。
