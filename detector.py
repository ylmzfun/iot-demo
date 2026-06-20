"""YOLO 物体识别封装（基于 ultralytics）。

在主机端对每一帧跑 YOLO，画出检测框、类别和置信度。
模型权重首次使用时由 ultralytics 自动下载（默认 yolov8n.pt，最快的 nano 版）。

设备自动选择：Apple Silicon 走 MPS，有 NVIDIA 走 CUDA，否则 CPU。
"""

from __future__ import annotations

import numpy as np


def _auto_device() -> str:
    try:
        import torch

        if torch.backends.mps.is_available():
            return "mps"  # Apple Silicon GPU
        if torch.cuda.is_available():
            return "0"  # 第一块 NVIDIA GPU
    except Exception:
        pass
    return "cpu"


class YoloDetector:
    """对单帧做目标检测并返回带标注的图像。"""

    def __init__(
        self,
        model_path: str = "yolov8n.pt",
        *,
        conf: float = 0.4,
        device: str | None = None,
    ) -> None:
        # 延迟到这里再 import，避免没装 ultralytics 时整个程序起不来
        from ultralytics import YOLO

        self.device = device or _auto_device()
        self.conf = conf
        self.model = YOLO(model_path)
        self.names = self.model.names
        print(f"[yolo] 模型 {model_path} 已加载，设备={self.device}")

        # 预热一次：触发权重下载 + 编译，避免第一帧卡顿
        warm = np.zeros((480, 640, 3), dtype=np.uint8)
        self.model.predict(warm, conf=self.conf, device=self.device, verbose=False)

    def annotate(self, frame: np.ndarray) -> tuple[np.ndarray, int]:
        """对 frame 做检测，返回 (标注后的图像, 检测到的目标数)。"""
        results = self.model.predict(
            frame, conf=self.conf, device=self.device, verbose=False
        )
        result = results[0]
        # ultralytics 自带的绘制：框 + 类别 + 置信度，风格统一
        annotated = result.plot()
        return annotated, len(result.boxes)
