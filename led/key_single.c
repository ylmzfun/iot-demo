/****************************************************************
 * 玩法2: 一个键点一个 LED (按键开关灯)
 *   清翔 QX-MCS51 V3.5 (STC89C52RC)
 *
 * 功能：
 *   16 个键映射到 8 个 LED (键号 % 8)。
 *   按一下 -> 对应 LED 亮; 再按一下 -> 灭 (翻转)。
 *
 * 接线同 main.c:  LED->P1   矩阵键盘->P3 (行P3.0-3 列P3.4-7)
 ****************************************************************/
#ifdef SDCC
#include <8052.h>
#else
#include <reg52.h>
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

#define LED      P1
#define KEYPORT  P3

void delay_ms(u16 t)
{
    u16 i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 114; j++);
}

u8 key_scan(void)
{
    u8 row, col, val, row_mask;
    for (row = 0; row < 4; row++)
    {
        row_mask = ~(1 << row);
        KEYPORT = row_mask | 0xF0;
        val = KEYPORT & 0xF0;
        if (val != 0xF0)
        {
            delay_ms(10);
            val = KEYPORT & 0xF0;
            if (val != 0xF0)
                for (col = 0; col < 4; col++)
                    if (!(val & (0x10 << col)))
                        return row * 4 + col;
        }
    }
    return 0xFF;
}

void main(void)
{
    u8 key;
    u8 state = 0x00;          /* 8 个 LED 的开关状态(1=亮), 内部用正逻辑 */

    LED = 0xFF;               /* 全灭 */

    while (1)
    {
        key = key_scan();
        if (key != 0xFF)
        {
            state ^= (1 << (key % 8));   /* 翻转对应 LED */
            LED = ~state;                /* 低电平点亮 */
            while (key_scan() != 0xFF);  /* 等松手 */
        }
    }
}
