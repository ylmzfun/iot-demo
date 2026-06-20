/****************************************************************
 * 流水灯程序  -  清翔 QX-MCS51 V3.5 开发板 (STC89C52RC)
 *
 * 硬件说明：
 *   流水灯模块 8 个 LED 接在 P1 口 (P1.0 ~ P1.7)
 *   LED 低电平点亮 (引脚 = 0 灯亮, = 1 灯灭)
 *   晶振 11.0592MHz
 ****************************************************************/
#ifdef SDCC
#include <8052.h>          /* SDCC 命令行编译用 */
#else
#include <reg52.h>         /* Keil C51 用 */
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

#define LED  P1            // 流水灯端口

/* 简单软件延时, 约 t 毫秒 (11.0592MHz 下近似) */
void delay_ms(u16 t)
{
    u16 i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 114; j++);
}

void main(void)
{
    u8 i;

    while (1)
    {
        /* 一个亮点从 P1.0 流向 P1.7 */
        LED = 0xFE;                 // 1111 1110  -> 只有 P1.0 亮
        for (i = 0; i < 8; i++)
        {
            delay_ms(150);          // 停留时间, 改这里调速度
            /* 把 0 循环左移一位: 亮点向高位移动, 到顶后回到 P1.0 */
            LED = (LED << 1) | 0x01;
            if (LED == 0xFF)        // 8 位走完, 重新开始
                LED = 0xFE;
        }
    }
}
