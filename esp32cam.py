"""ESP32-S 摄像头 MJPEG 视频流读取模块。

ESP32-S 烧录 CameraWebServer 类固件后，会在局域网内提供一个
multipart/x-mixed-replace 的 MJPEG 视频流（默认 http://<ip>:81/stream）。

这里用 requests 手动解析 multipart 流，逐帧用 OpenCV 解码成 BGR 图像。
相比直接 cv2.VideoCapture(url)，手动解析不依赖 OpenCV 的 FFMPEG 后端，
对 ESP32-CAM 这种简单 MJPEG 流更稳定、延迟更低。
"""

from __future__ import annotations

import time
from collections.abc import Iterator
from urllib.parse import urlparse

import cv2
import numpy as np
import requests

# JPEG 帧的起止标记
_SOI = b"\xff\xd8"  # Start Of Image
_EOI = b"\xff\xd9"  # End Of Image


def build_stream_url(target: str) -> str:
    """把用户输入规整成完整的 MJPEG 流地址。

    支持几种写法，都会归一化到 http://<host>:81/stream：
      - 192.168.1.50
      - http://192.168.1.50
      - http://192.168.1.50:81/stream  （已是完整地址则原样返回）
    """
    target = target.strip()
    if not target.startswith(("http://", "https://")):
        target = "http://" + target

    parsed = urlparse(target)
    # 已经带了路径（例如 /stream、/capture），认为是完整地址，直接用
    if parsed.path and parsed.path != "/":
        return target

    host = parsed.hostname or ""
    # 没指定端口时，ESP32-CAM 的视频流默认在 81 端口
    port = parsed.port or 81
    return f"http://{host}:{port}/stream"


class MJPEGCamera:
    """从 ESP32-S 的 MJPEG 流中逐帧读取图像。

    用法::

        cam = MJPEGCamera("http://192.168.1.50:81/stream")
        for frame in cam.frames():
            cv2.imshow("preview", frame)
    """

    def __init__(
        self,
        url: str,
        *,
        timeout: float = 10.0,
        chunk_size: int = 4096,
        reconnect_delay: float = 2.0,
    ) -> None:
        self.url = build_stream_url(url)
        self.timeout = timeout
        self.chunk_size = chunk_size
        self.reconnect_delay = reconnect_delay

    def frames(self) -> Iterator[np.ndarray]:
        """持续产出解码后的 BGR 帧；连接断开时自动重连。"""
        while True:
            try:
                yield from self._read_stream()
            except (requests.RequestException, ConnectionError) as exc:
                print(f"[esp32cam] 连接中断：{exc}；{self.reconnect_delay:.0f}s 后重连…")
                time.sleep(self.reconnect_delay)

    def _read_stream(self) -> Iterator[np.ndarray]:
        with requests.get(self.url, stream=True, timeout=self.timeout) as resp:
            resp.raise_for_status()
            print(f"[esp32cam] 已连接：{self.url}")
            buffer = bytearray()
            for chunk in resp.iter_content(chunk_size=self.chunk_size):
                if not chunk:
                    continue
                buffer.extend(chunk)
                # 一个缓冲区里可能攒了多帧，逐帧取出
                while True:
                    start = buffer.find(_SOI)
                    if start == -1:
                        # 还没看到帧头，丢弃多余的边界/头部数据，避免无限增长
                        if len(buffer) > 1_000_000:
                            del buffer[:-2]
                        break
                    end = buffer.find(_EOI, start + 2)
                    if end == -1:
                        # 帧还没接收完整，丢掉帧头之前的垃圾，等下一个 chunk
                        del buffer[:start]
                        break
                    jpg = bytes(buffer[start : end + 2])
                    del buffer[: end + 2]
                    frame = cv2.imdecode(
                        np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR
                    )
                    if frame is not None:
                        yield frame
