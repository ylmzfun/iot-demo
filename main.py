"""ESP32-S 摄像头实时预览（可选 YOLO 物体识别）。

从 ESP32-S 的 WiFi MJPEG 视频流读取画面，用 OpenCV 窗口实时显示，
并可叠加 YOLO 目标检测框。

运行::

    uv run main.py --url 192.168.1.50            # 默认开启 YOLO 识别
    uv run main.py --url 192.168.1.50 --no-detect # 只预览，不识别
    ESP32_CAM_URL=192.168.1.50 uv run main.py     # 用环境变量指定地址

窗口快捷键：
    q / Esc   退出
    s         保存当前帧为 jpg
    d         开/关 YOLO 识别
"""

from __future__ import annotations

import argparse
import os
import time

import cv2

from esp32cam import MJPEGCamera


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ESP32-S 摄像头实时预览 + YOLO 识别")
    parser.add_argument(
        "--url",
        default=os.environ.get("ESP32_CAM_URL"),
        help="ESP32-S 地址，可写 IP（192.168.1.50）或完整流地址"
        "（http://192.168.1.50:81/stream）。也可用环境变量 ESP32_CAM_URL。",
    )
    parser.add_argument(
        "--window",
        default="ESP32-S Camera",
        help="预览窗口标题",
    )
    detect_grp = parser.add_mutually_exclusive_group()
    detect_grp.add_argument(
        "--detect",
        dest="detect",
        action="store_true",
        help="开启 YOLO 物体识别（默认开启）",
    )
    detect_grp.add_argument(
        "--no-detect",
        dest="detect",
        action="store_false",
        help="关闭 YOLO，只做预览",
    )
    parser.set_defaults(detect=True)
    parser.add_argument(
        "--model",
        default="yolov8n.pt",
        help="YOLO 模型权重，首次会自动下载（n/s/m/l/x 越大越准越慢）",
    )
    parser.add_argument(
        "--conf",
        type=float,
        default=0.4,
        help="检测置信度阈值（0~1），越高框越少越准",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.url:
        raise SystemExit(
            "未指定摄像头地址。请用 --url 192.168.1.50 "
            "或设置环境变量 ESP32_CAM_URL。"
        )

    # 按需加载 YOLO（关闭识别时完全不碰 ultralytics/torch）
    detector = None
    detect_on = args.detect
    if detect_on:
        from detector import YoloDetector

        print(f"[main] 正在加载 YOLO 模型 {args.model} …（首次会下载权重）")
        detector = YoloDetector(args.model, conf=args.conf)

    cam = MJPEGCamera(args.url)
    print(f"[main] 正在连接 {cam.url} … 按 q/Esc 退出，s 截图，d 开关识别。")

    fps = 0.0
    last_t = time.monotonic()

    for frame in cam.frames():
        now = time.monotonic()
        dt = now - last_t
        last_t = now
        if dt > 0:
            fps = 0.9 * fps + 0.1 * (1.0 / dt) if fps else 1.0 / dt

        n_obj = 0
        if detect_on and detector is not None:
            frame, n_obj = detector.annotate(frame)

        status = f"{fps:4.1f} FPS  {frame.shape[1]}x{frame.shape[0]}"
        if detect_on and detector is not None:
            status += f"  YOLO:{n_obj}"
        else:
            status += "  YOLO:off"
        cv2.putText(
            frame,
            status,
            (10, 24),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (0, 255, 0),
            2,
            cv2.LINE_AA,
        )
        cv2.imshow(args.window, frame)

        key = cv2.waitKey(1) & 0xFF
        if key in (ord("q"), 27):  # q 或 Esc
            break
        if key == ord("s"):
            fname = f"snapshot_{int(now)}.jpg"
            cv2.imwrite(fname, frame)
            print(f"[main] 已保存 {fname}")
        if key == ord("d"):
            if detector is None:
                # 第一次开启时才真正加载模型
                from detector import YoloDetector

                print(f"[main] 加载 YOLO 模型 {args.model} …")
                detector = YoloDetector(args.model, conf=args.conf)
            detect_on = not detect_on
            print(f"[main] YOLO 识别：{'开' if detect_on else '关'}")

        if cv2.getWindowProperty(args.window, cv2.WND_PROP_VISIBLE) < 1:
            break

    cv2.destroyAllWindows()
    print("[main] 已退出。")


if __name__ == "__main__":
    main()
