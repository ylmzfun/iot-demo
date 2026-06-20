# ---- STM32F103C8 裸机闪烁 Makefile ----
# 依赖: arm-none-eabi-gcc (编译) + openocd (烧录)
#   brew install arm-none-eabi-gcc
#   brew install openocd        # 你已经装了

TARGET  = blink

# 工具链路径：默认用解压到 ~/opt 的 Arm GNU Toolchain；
# 若你以后把工具链装到系统 PATH，可改成空：  TOOLCHAIN_BIN =
TOOLCHAIN_BIN ?= $(HOME)/opt/arm-none-eabi-14.2/bin/
CROSS   = $(TOOLCHAIN_BIN)arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

# 源文件
SRCS = Core/Src/main.c \
       Core/Startup/startup_stm32f103.c

LDSCRIPT = STM32F103C8Tx_FLASH.ld

# Cortex-M3 编译选项；裸机环境 -nostdlib，不链接标准库的启动代码
CFLAGS  = -mcpu=cortex-m3 -mthumb \
          -Wall -Wextra -O2 -g3 \
          -ffreestanding -fno-common -ffunction-sections -fdata-sections

LDFLAGS = -mcpu=cortex-m3 -mthumb \
          -T $(LDSCRIPT) -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map

.PHONY: all clean flash

all: $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(SRCS) $(LDSCRIPT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# 通过 ST-Link + OpenOCD 烧录并复位运行
flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	        -c "program $(TARGET).elf verify reset exit"

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map
