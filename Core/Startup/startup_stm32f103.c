/**
  ******************************************************************************
  * @file    startup_stm32f103.c
  * @brief   STM32F103 最小启动文件 (C 实现)
  *          - 定义中断向量表 (放在 Flash 起始处)
  *          - Reset_Handler: 初始化 .data / .bss 后跳转 main
  *  本例只用到忙等延时、不开任何中断，因此外设中断全部指向 Default_Handler。
  ******************************************************************************
  */
#include <stdint.h>

/* 链接脚本中定义的符号 */
extern uint32_t _sidata;   /* .data 在 Flash 中的加载地址 */
extern uint32_t _sdata;    /* .data 段 RAM 起始 */
extern uint32_t _edata;    /* .data 段 RAM 结束 */
extern uint32_t _sbss;     /* .bss 段起始 */
extern uint32_t _ebss;     /* .bss 段结束 */
extern uint32_t _estack;   /* 栈顶 (RAM 末尾) */

extern int main(void);

/* 复位后第一个执行的函数 */
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    /* 把 .data 的初值从 Flash 拷到 RAM */
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* .bss 清零 */
    for (dst = &_sbss; dst < &_ebss; dst++)
    {
        *dst = 0U;
    }

    main();

    /* main 不应返回，万一返回就死循环 */
    for (;;) { }
}

/* 未使用的异常/中断统一停在这里，便于调试时定位 */
void Default_Handler(void)
{
    for (;;) { }
}

/* 让其他异常处理弱链接到 Default_Handler，需要时可自行重定义 */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* Cortex-M3 核心异常向量表 (16 项)，足够跑这个无中断的例子 */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))&_estack,  /* 0  初始栈指针 */
    Reset_Handler,             /* 1  复位        */
    NMI_Handler,               /* 2  NMI         */
    HardFault_Handler,         /* 3  HardFault   */
    MemManage_Handler,         /* 4              */
    BusFault_Handler,          /* 5              */
    UsageFault_Handler,        /* 6              */
    0, 0, 0, 0,                /* 7-10 保留      */
    SVC_Handler,               /* 11 SVCall      */
    DebugMon_Handler,          /* 12             */
    0,                         /* 13 保留        */
    PendSV_Handler,            /* 14 PendSV      */
    SysTick_Handler,           /* 15 SysTick     */
};
