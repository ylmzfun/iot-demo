/* STM32F103C8T6 链接脚本
 * Flash: 64KB @ 0x08000000   RAM: 20KB @ 0x20000000
 * (若你的板子是 C8 被识别成 128KB 的 CB，可把 FLASH LENGTH 改成 128K)
 */
ENTRY(Reset_Handler)

/* 栈顶 = RAM 末尾，向下生长 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    /* 中断向量表必须放在 Flash 起始 (0x08000000) */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* 代码与只读数据 */
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* .data 加载地址 (在 Flash 中的位置) */
    _sidata = LOADADDR(.data);

    /* 已初始化的全局变量：运行在 RAM，初值存 Flash，启动时拷贝 */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* 未初始化的全局变量：启动时清零 */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
