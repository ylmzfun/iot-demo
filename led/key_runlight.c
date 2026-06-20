/****************************************************************
 * 玩法3: 按键控制流水灯 (启停 / 调速 / 换向)
 *   清翔 QX-MCS51 V3.5 (STC89C52RC)
 *
 * 上电后 P1 自动跑流水灯(一个亮点移动)。用矩阵键盘控制:
 *   键 0 : 启动/暂停 (toggle)
 *   键 1 : 加速
 *   键 2 : 减速
 *   键 3 : 改变方向
 *   其余键: 忽略
 *
 * 接线:  LED->P1   矩阵键盘->P3 (行P3.0-3 列P3.4-7)
 * 说明: 没接键盘也能看到流水灯自动跑, 接上键盘后即可控制。
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

/* 单次扫描(含消抖), 返回键号 0~15 或 0xFF */
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
    u8  pos    = 0;      /* 亮点位置 0~7 */
    u8  dir    = 0;      /* 0=左移(向高位) 1=右移 */
    u8  paused = 0;      /* 0=运行 1=暂停 */
    u8  speed  = 30;     /* 速度: 数值越大越慢(单位 ~5ms) */
    u16 cnt    = 0;
    u8  key;

    while (1)
    {
        key = key_scan();
        if (key != 0xFF)
        {
            switch (key)
            {
                case 0: paused = !paused;                break;  /* 启停 */
                case 1: if (speed > 6)  speed -= 6;      break;  /* 加速 */
                case 2: if (speed < 90) speed += 6;      break;  /* 减速 */
                case 3: dir = !dir;                      break;  /* 换向 */
                default: break;
            }
            while (key_scan() != 0xFF);   /* 等松手 */
        }

        LED = ~(1 << pos);                /* 显示当前亮点(低电平点亮) */

        if (!paused)
        {
            if (++cnt >= speed)           /* 到时间才走一步 */
            {
                cnt = 0;
                if (dir == 0)
                    pos = (pos + 1) & 0x07;   /* 左移循环 */
                else
                    pos = (pos - 1) & 0x07;   /* 右移循环 */
            }
        }
        delay_ms(5);
    }
}
