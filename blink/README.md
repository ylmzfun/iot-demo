# 51 单片机流水灯 — 命令行编译烧录

清翔 QX-MCS51 V3.5 开发板，主控 **STC89C52RC**，macOS 下纯命令行完成编译与烧录（不依赖 Keil / STC-ISP）。

## 硬件说明

- 开发板：清翔电子 QX-MCS51 V3.5
- 单片机：STC89C52RC（8KB Flash，11.0592MHz 晶振）
- 流水灯模块：8 个 LED 接 **P1 口（P1.0~P1.7）**，**低电平点亮**
- 下载方式：板载 CH340 USB 转串口，macOS 下识别为 `/dev/cu.usbserial-210`

## 工具链

| 用途 | 工具 | 安装方式 |
|------|------|----------|
| C 编译器 | SDCC 4.5.0 | 官方 macOS 预编译包（解压即用） |
| STC 烧录 | stcgal 1.10 | `pip3 install --user stcgal` |

### 安装 SDCC（一次性）

```bash
cd /tmp
curl -L -o sdcc.tar.bz2 \
  "https://master.dl.sourceforge.net/project/sdcc/sdcc-macos-amd64/4.5.0/sdcc-4.5.0-x86_64-apple-macosx.tar.bz2?viasf=1"
tar -xjf sdcc.tar.bz2
# 编译器在: /tmp/sdcc-extract/sdcc-4.5.0/bin
```

> 注意：`/tmp` 重启会被清空。要长期使用，把 `sdcc-4.5.0` 目录移到固定位置
> （例如 `~/tools/sdcc-4.5.0`），并把其 `bin` 加入 PATH。

### 安装 stcgal（一次性）

```bash
pip3 install --user stcgal
# 用法: python3 -m stcgal ...
```

## 编译

```bash
export PATH="/tmp/sdcc-extract/sdcc-4.5.0/bin:$PATH"
sdcc -mmcs51 main.c            # 生成 main.ihx
packihx main.ihx > main.hex    # 转成标准 Intel HEX
```

源码用 `#ifdef SDCC` 做了兼容，SDCC 走 `<8052.h>`，Keil C51 走 `<reg52.h>`，同一份 `main.c` 两边都能编。

## 烧录

STC 芯片是**冷启动下载**：运行烧录命令后程序会等待，需把板子上的**电源开关 OFF → ON 拨一下**（断电重上电）才能进入下载模式。

```bash
python3 -m stcgal -P stc89 -p /dev/cu.usbserial-210 main.hex
```

看到 `Waiting for MCU, please cycle power` 时拨电源开关，出现
`Writing flash 100% ... Disconnected!` 即烧录成功。

> 串口名可能不同，先用 `ls /dev/cu.*` 确认 CH340 对应的端口。

## 一键编译 + 烧录

```bash
cd /Users/ylmzfun/Public/train/iot/80c51
export PATH="/tmp/sdcc-extract/sdcc-4.5.0/bin:$PATH"
sdcc -mmcs51 main.c && packihx main.ihx > main.hex \
  && python3 -m stcgal -P stc89 -p /dev/cu.usbserial-210 main.hex
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 流水灯源码（Keil / SDCC 通用） |
| `main.hex` | 编译产物，烧录用 |
| `main.ihx` | SDCC 中间 HEX（由 packihx 转 main.hex） |
| `README.md` | 本文档 |

## 效果

一个亮点从 P1.0 依次流向 P1.7 循环往复。改速度调 `main.c` 里 `delay_ms(150)` 的数值，越小越快。
