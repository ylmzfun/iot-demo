/****************************************************************
 * 矩阵键盘控制 LED  -  清翔 QX-MCS51 V3.5 开发板 (STC89C52RC)
 *
 * 功能：
 *   按下 4x4 矩阵键盘的某个键(编号 0~15)，
 *   P1 口的 8 个 LED 以二进制显示该键编号(灯亮 = 1)。
 *
 * 接线：
 *   流水灯 LED  -> P1 (P1.0~P1.7, 低电平点亮, 板载固定)
 *   矩阵键盘    -> P3 (用杜邦线把键盘 8 脚接到 P3.0~P3.7)
 *                 行 ROW = P3.0~P3.3, 列 COL = P3.4~P3.7
 *   晶振 11.0592MHz
 *
 * 若你的矩阵键盘实际接在别的口, 只改下面 KEYPORT 一行即可。
 ****************************************************************/
#ifdef SDCC
#include <8052.h>          /* SDCC 命令行编译用 */
#else
#include <reg52.h>         /* Keil C51 用 */
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

#define LED      P1        /* 8 个 LED, 低电平点亮 */
#define KEYPORT  P3        /* 矩阵键盘端口 */

void delay_ms(u16 t)
{
    u16 i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 114; j++);
}

/* 扫描矩阵键盘, 返回键编号 0~15, 无键按下返回 0xFF
 * 行 P3.0~P3.3 逐个输出低, 读列 P3.4~P3.7 */
u8 key_scan(void)
{
    u8 row, col, val;
    u8 row_mask;

    for (row = 0; row < 4; row++)
    {
        /* 当前行输出 0, 其余行和所有列输出 1 */
        row_mask = ~(1 << row);          /* 只有该行为 0 */
        KEYPORT = row_mask | 0xF0;       /* 高 4 位(列)置 1 准备读 */

        val = KEYPORT & 0xF0;            /* 读列 */
        if (val != 0xF0)                 /* 有某列被拉低 -> 有键按下 */
        {
            delay_ms(10);                /* 消抖 */
            val = KEYPORT & 0xF0;
            if (val != 0xF0)
            {
                for (col = 0; col < 4; col++)
                    if (!(val & (0x10 << col)))   /* 找出被拉低的列 */
                        return row * 4 + col;     /* 键编号 0~15 */
            }
        }
    }
    return 0xFF;                          /* 无键按下 */
}

void main(void)
{
    u8 key;

    LED = 0xFF;                           /* 开机所有 LED 灭 */

    while (1)
    {
        key = key_scan();
        if (key != 0xFF)                  /* 有键按下 */
        {
            LED = ~key;                   /* 二进制显示编号(灯亮=1) */
            while (key_scan() != 0xFF);   /* 等待松手, 防连按 */
        }
    }
}
