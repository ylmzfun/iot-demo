/****************************************************************
 * 光敏 ADC 读数调试  -  数码管显示 PCF8591 AIN1 的值 (0~255)
 *   清翔 QX-MCS51 V3.5 (STC89C52RC)
 *
 * 用途: 看数码管上的数字判断 I2C 和明暗
 *   - 数字随光照变化 -> I2C 正常, 记下"亮"和"暗"的读数定阈值
 *   - 数字卡死在 255(或 000) -> I2C 没通, 把 SCL/SDA(P2.1/P2.0) 对调
 *
 * 接线: 数码管 P0+P2.6/P2.7(板载) ; PCF8591 I2C P2.1=SCL P2.0=SDA
 ****************************************************************/
#ifdef SDCC
#include <8052.h>
#else
#include <reg52.h>
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

/* 数码管锁存 */
#define DUAN_H()  (P2 |=  0x40)
#define DUAN_L()  (P2 &= ~0x40)
#define WEI_H()   (P2 |=  0x80)
#define WEI_L()   (P2 &= ~0x80)

/* I2C 引脚 (对调版: P2.0=SCL, P2.1=SDA) */
#define SCL_1()  (P2 |=  0x01)
#define SCL_0()  (P2 &= ~0x01)
#define SDA_1()  (P2 |=  0x02)
#define SDA_0()  (P2 &= ~0x02)
#define SDA_IN() (P2 &   0x02)

#define LIGHT_CH  0x01

const u8 SEG[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
u8 disp[8];

void delay_ms(u16 t){ u16 i,j; for(i=0;i<t;i++) for(j=0;j<114;j++); }
void i2c_delay(void){ u8 i; for(i=0;i<8;i++); }

void show_once(void)
{
    u8 i;
    for (i = 0; i < 8; i++)
    {
        P0 = 0x00;      DUAN_H(); DUAN_L();
        P0 = ~(1 << i); WEI_H();  WEI_L();
        P0 = disp[i];   DUAN_H(); DUAN_L();
        delay_ms(1);
    }
}

void i2c_start(void){ SDA_1(); SCL_1(); i2c_delay(); SDA_0(); i2c_delay(); SCL_0(); i2c_delay(); }
void i2c_stop(void){ SDA_0(); SCL_1(); i2c_delay(); SDA_1(); i2c_delay(); }

u8 i2c_write(u8 dat)
{
    u8 i, ack;
    for (i = 0; i < 8; i++){
        if (dat & 0x80) SDA_1(); else SDA_0();
        dat <<= 1; i2c_delay();
        SCL_1(); i2c_delay(); SCL_0(); i2c_delay();
    }
    SDA_1(); i2c_delay(); SCL_1(); i2c_delay();
    ack = SDA_IN() ? 1 : 0;
    SCL_0(); i2c_delay();
    return ack;
}

u8 i2c_read(u8 ack)
{
    u8 i, dat = 0;
    SDA_1();
    for (i = 0; i < 8; i++){
        SCL_1(); i2c_delay();
        dat <<= 1; if (SDA_IN()) dat |= 0x01;
        SCL_0(); i2c_delay();
    }
    if (ack) SDA_0(); else SDA_1();
    i2c_delay(); SCL_1(); i2c_delay(); SCL_0(); i2c_delay(); SDA_1();
    return dat;
}

u8 adc_read(u8 ch)
{
    u8 v;
    i2c_start();
    i2c_write(0x90);
    i2c_write(0x40 | ch);
    i2c_start();
    i2c_write(0x91);
    v = i2c_read(1);
    v = i2c_read(0);
    i2c_stop();
    return v;
}

void main(void)
{
    u8 v, k;
    u8 i;

    for (i = 0; i < 8; i++) disp[i] = 0x00;

    while (1)
    {
        v = adc_read(LIGHT_CH);

        /* 右边三位显示十进制读数 0~255 */
        disp[5] = SEG[v / 100];
        disp[6] = SEG[(v / 10) % 10];
        disp[7] = SEG[v % 10];

        for (k = 0; k < 25; k++) show_once();   /* 刷新约 0.2s 再采一次 */
    }
}
