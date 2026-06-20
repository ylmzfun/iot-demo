/****************************************************************
 * 光控流水灯  -  清翔 QX-MCS51 V3.5 (STC89C52RC)
 *
 * 功能: 读 ADDA 模块的光敏电阻(PCF8591 ADC)
 *        无光(暗) -> P1 流水灯流动闪烁
 *        有光(亮) -> P1 全灭
 *
 * 硬件:
 *   流水灯 LED -> P1 (低电平点亮, 板载固定)
 *   PCF8591    -> I2C 软件模拟:  SCL=P2.1  SDA=P2.0
 *                 器件地址 0x90(写)/0x91(读)
 *                 光敏电阻接 AIN1 通道
 *                 (ADDA 模块上把光敏对应的跳线接好)
 *   晶振 11.0592MHz
 *
 * 若读不到/方向反, 见文件末尾和 README 的调试说明。
 ****************************************************************/
#ifdef SDCC
#include <8052.h>
#else
#include <reg52.h>
#endif

typedef unsigned char  u8;
typedef unsigned int   u16;

#define LED        P1

/* ---- I2C 引脚 (P2.1=SCL, P2.0=SDA), 读改写, Keil/SDCC 通用 ---- */
#define SCL_1()  (P2 |=  0x02)
#define SCL_0()  (P2 &= ~0x02)
#define SDA_1()  (P2 |=  0x01)
#define SDA_0()  (P2 &= ~0x01)
#define SDA_IN() (P2 &   0x01)

/* ---- 光控参数 (按实物微调) ---- */
#define LIGHT_CH     0x01    /* 光敏所在通道 AIN1 */
#define THRESHOLD    100     /* 明暗判别阈值 0~255 */
#define DARK_IS_HIGH 0       /* 1:暗时ADC值大  0:暗时ADC值小, 反了就改这里 */

void delay_ms(u16 t)
{
    u16 i, j;
    for (i = 0; i < t; i++)
        for (j = 0; j < 114; j++);
}

void i2c_delay(void) { u8 i; for (i = 0; i < 8; i++); }

void i2c_start(void)
{
    SDA_1(); SCL_1(); i2c_delay();
    SDA_0(); i2c_delay();
    SCL_0(); i2c_delay();
}

void i2c_stop(void)
{
    SDA_0(); SCL_1(); i2c_delay();
    SDA_1(); i2c_delay();
}

/* 写一字节, 返回应答(0=ACK) */
u8 i2c_write(u8 dat)
{
    u8 i, ack;
    for (i = 0; i < 8; i++)
    {
        if (dat & 0x80) SDA_1(); else SDA_0();
        dat <<= 1;
        i2c_delay();
        SCL_1(); i2c_delay(); SCL_0(); i2c_delay();
    }
    SDA_1(); i2c_delay();            /* 释放总线读应答 */
    SCL_1(); i2c_delay();
    ack = SDA_IN() ? 1 : 0;
    SCL_0(); i2c_delay();
    return ack;
}

/* 读一字节, ack=1 发应答继续读, 0 发非应答结束 */
u8 i2c_read(u8 ack)
{
    u8 i, dat = 0;
    SDA_1();                         /* 释放总线 */
    for (i = 0; i < 8; i++)
    {
        SCL_1(); i2c_delay();
        dat <<= 1;
        if (SDA_IN()) dat |= 0x01;
        SCL_0(); i2c_delay();
    }
    if (ack) SDA_0(); else SDA_1();
    i2c_delay();
    SCL_1(); i2c_delay(); SCL_0(); i2c_delay();
    SDA_1();
    return dat;
}

/* 读 PCF8591 指定通道 (0~3), 返回 0~255 */
u8 adc_read(u8 ch)
{
    u8 v;
    i2c_start();
    i2c_write(0x90);                 /* 写地址 */
    i2c_write(0x40 | ch);            /* 控制字: 使能输出 + 选通道 */
    i2c_start();
    i2c_write(0x91);                 /* 读地址 */
    v = i2c_read(1);                 /* 第一次为上次结果, 丢弃 */
    v = i2c_read(0);                 /* 本次有效值 */
    i2c_stop();
    return v;
}

u8 is_dark(u8 v)
{
#if DARK_IS_HIGH
    return (v > THRESHOLD);
#else
    return (v < THRESHOLD);
#endif
}

void main(void)
{
    u8 pos = 0;
    u8 v;

    LED = 0xFF;                      /* 开机全灭 */

    while (1)
    {
        v = adc_read(LIGHT_CH);

        if (is_dark(v))              /* 暗: 跑流水灯 */
        {
            LED = ~(1 << pos);
            pos = (pos + 1) & 0x07;
        }
        else                        /* 亮: 全灭 */
        {
            LED = 0xFF;
        }
        delay_ms(120);              /* 流动速度 / 采样间隔 */
    }
}
