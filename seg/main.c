/****************************************************************
 * 矩阵键盘输入数字 -> 数码管显示
 *   清翔 QX-MCS51 V3.5 开发板 (STC89C52RC)
 *
 * 硬件 (板载固定):
 *   数码管 8 位, 共阴极, 由 2 片 74HC573 锁存驱动
 *     P0      = 数据总线(分时送段码/位码)
 *     P2.6    = 段选锁存(DUAN), 高透明 低锁存
 *     P2.7    = 位选锁存(WEI),  高透明 低锁存
 *     共阴: 段码高电平点亮; 位选低电平选中该位
 *
 * 矩阵键盘 -> P3 (杜邦线): 行 P3.0~P3.3, 列 P3.4~P3.7
 *   键号 0~9 : 输入对应数字(从右侧移入)
 *   键号 10~15: 清屏归零
 *   (键号 = 行×4+列, 按一下即可看出和丝印的对应关系)
 *
 * 晶振 11.0592MHz
 ****************************************************************/
#ifdef SDCC
#include <8052.h>
#else
#include <reg52.h>
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

#define KEYPORT  P3

/* P2.6=段选  P2.7=位选, 用读改写避免依赖 sbit, Keil/SDCC 通用 */
#define DUAN_H()  (P2 |=  0x40)
#define DUAN_L()  (P2 &= ~0x40)
#define WEI_H()   (P2 |=  0x80)
#define WEI_L()   (P2 &= ~0x80)

/* 共阴段码 0~9 (bit: dp g f e d c b a) */
const u8 SEG[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};

u8 disp[8];        /* 8 位的段码, disp[0]=位选 0xFE 那一位 */

void delay_ms(u16 t)
{
    u16 i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 114; j++);
}

/* 把 8 位扫描显示一遍 (动态扫描) */
void show_once(void)
{
    u8 i;
    for (i = 0; i < 8; i++)
    {
        P0 = 0x00;  DUAN_H(); DUAN_L();   /* 先消隐, 防鬼影 */
        P0 = ~(1 << i); WEI_H(); WEI_L(); /* 选中第 i 位(低有效) */
        P0 = disp[i];   DUAN_H(); DUAN_L();/* 送段码 */
        delay_ms(1);
    }
}

/* 扫描矩阵键盘, 返回键号 0~15 或 0xFF; 期间保持数码管刷新 */
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
            show_once();                  /* 用刷新代替消抖延时, 屏不灭 */
            KEYPORT = row_mask | 0xF0;
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
    u8 key, i;

    for (i = 0; i < 8; i++) disp[i] = 0x00;   /* 开机全灭 */

    while (1)
    {
        show_once();
        key = key_scan();
        if (key != 0xFF)
        {
            if (key <= 9)                 /* 数字键: 左移一位, 右侧填入新数字 */
            {
                for (i = 0; i < 7; i++) disp[i] = disp[i + 1];
                disp[7] = SEG[key];
            }
            else                          /* 其余键: 清屏 */
            {
                for (i = 0; i < 8; i++) disp[i] = 0x00;
            }
            while (key_scan() != 0xFF) show_once();   /* 等松手, 屏不灭 */
        }
    }
}
