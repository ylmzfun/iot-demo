# 矩阵键盘控制 LED

清翔 QX-MCS51 V3.5 开发板 / STC89C52RC。矩阵键盘 + 流水灯 LED 综合实验，命令行编译烧录。

## 接线

| 模块 | 端口 | 说明 |
|------|------|------|
| 流水灯 8×LED | **P1** (P1.0~P1.7) | 板载固定，低电平点亮 |
| 4×4 矩阵键盘 | **P3** (P3.0~P3.7) | 用杜邦线把键盘 8 脚接到 P3：行 ROW=P3.0~P3.3，列 COL=P3.4~P3.7 |

> 矩阵键盘放在 P3 是为了**避开 P1 流水灯的冲突**（本板 LED 固定在 P1）。
> 如果你的键盘接在别的口，只改源码里 `#define KEYPORT P3` 一行即可。
>
> 键号 0~15 按「行×4+列」计算，和丝印 S6~S21 的物理顺序不一定一致，
> 烧进去按一下即可看出对应关系。

## 三个玩法

| 源码 | 功能 |
|------|------|
| `main.c` | **二进制显示**：按某键，8 个 LED 用二进制亮出键号 0~15（灯亮=1）。例：按到键 5 → P1.0、P1.2 亮(101) |
| `key_single.c` | **一键一灯**：键号 % 8 对应一个 LED，按一下亮、再按一下灭（翻转） |
| `key_runlight.c` | **流水灯控制**：上电自动跑流水灯；键0 启停、键1 加速、键2 减速、键3 换向 |

## 编译

```bash
export PATH="/tmp/sdcc-extract/sdcc-4.5.0/bin:$PATH"
sdcc -mmcs51 main.c && packihx main.ihx > main.hex
# 换玩法把 main 换成 key_single / key_runlight 即可
```

## 烧录

接好 P3 键盘后：

```bash
python3 -m stcgal -P stc89 -p /dev/cu.usbserial-210 main.hex
```

运行后把板子**电源开关 OFF→ON 拨一下**（冷启动）进入下载，看到 `Writing flash 100% ... Disconnected!` 即成功。串口名可用 `ls /dev/cu.*` 确认。

## 一键编译+烧录（以 key_runlight 为例）

```bash
cd /Users/ylmzfun/Public/train/iot/80c51/led
export PATH="/tmp/sdcc-extract/sdcc-4.5.0/bin:$PATH"
sdcc -mmcs51 key_runlight.c && packihx key_runlight.ihx > key_runlight.hex \
  && python3 -m stcgal -P stc89 -p /dev/cu.usbserial-210 key_runlight.hex
```

## 矩阵键盘扫描原理

逐行扫描法：行 P3.0~P3.3 轮流输出低电平、其余为高，同时读列 P3.4~P3.7。
若某列被拉低，说明该行该列交叉处的键被按下，键号 = 行×4 + 列。
读到后做 10ms 消抖，并等待松手防止连按。
