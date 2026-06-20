/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8 (Blue Pill) 双灯反相交替呼吸灯 (gamma 校正)
  *
  *  - D2 = PC13 (板载灯, 低电平点亮)
  *  - 外接 LED = PB0 (高电平点亮; 220Ω 限流到 GND)
  *  - 两个灯此消彼长地交替呼吸: 一个渐亮时另一个渐暗
  *  - PC13 不是定时器 PWM 引脚, 用软件 PWM (多通道, 同一周期内分别关断)
  *  - 时间上线性步进, 占空比按 gamma=2.2 映射, 贴近人眼亮度感受
  ******************************************************************************
  */
#include <stdint.h>

/* ---- 寄存器地址 (来自 STM32F103 参考手册 RM0008) -------------------------- */
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021018U)  /* APB2 外设时钟使能  */
#define GPIOB_CRL     (*(volatile uint32_t *)0x40010C00U)  /* GPIOB 低 8 位配置   */
#define GPIOB_ODR     (*(volatile uint32_t *)0x40010C0CU)  /* GPIOB 输出数据      */
#define GPIOC_CRH     (*(volatile uint32_t *)0x40011004U)  /* GPIOC 高 8 位配置   */
#define GPIOC_ODR     (*(volatile uint32_t *)0x4001100CU)  /* GPIOC 输出数据      */

#define RCC_IOPBEN    (1U << 3)   /* GPIOB 时钟使能位 */
#define RCC_IOPCEN    (1U << 4)   /* GPIOC 时钟使能位 */

#define LEDA_PIN      13U         /* PC13, D2, 低电平点亮 */
#define LEDB_PIN      0U          /* PB0, 外接, 高电平点亮 */

/* ---- 呼吸灯参数 (可调) ----------------------------------------------------
 *  PWM_MAX : PWM 占空比满量程 (查找表最大值)
 *  UNIT    : 每个 PWM 子步的忙等时长 -> 越大 PWM 频率越低 (太低会看到闪)
 *  HOLD    : 每个亮度档维持多少个 PWM 周期 -> 越大呼吸越慢
 */
#define PWM_MAX  255U
#define UNIT     35U
#define HOLD     6U

/* gamma=2.2 查找表 (64 档): ((i/63)^2.2)*255, 时间等间隔走表 -> 人眼线性 */
static const uint8_t gamma_lut[64] =
{
      0,   0,   0,   0,   1,   1,   1,   2,
      3,   4,   4,   5,   7,   8,   9,  11,
     13,  14,  16,  18,  20,  23,  25,  28,
     31,  33,  36,  40,  43,  46,  50,  54,
     57,  61,  66,  70,  74,  79,  84,  89,
     94,  99, 105, 110, 116, 122, 128, 134,
    140, 147, 153, 160, 167, 174, 182, 189,
    197, 205, 213, 221, 229, 238, 246, 255,
};
#define LUT_LEN  (sizeof(gamma_lut) / sizeof(gamma_lut[0]))

static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm__ volatile ("nop");
    }
}

/* PC13 低电平点亮 */
static void ledA_on(void)  { GPIOC_ODR &= ~(1U << LEDA_PIN); }
static void ledA_off(void) { GPIOC_ODR |=  (1U << LEDA_PIN); }
/* PB0 高电平点亮 */
static void ledB_on(void)  { GPIOB_ODR |=  (1U << LEDB_PIN); }
static void ledB_off(void) { GPIOB_ODR &= ~(1U << LEDB_PIN); }

/* 双通道软件 PWM: 在同一个周期里按各自占空比 dutyA / dutyB 关断 */
static void pwm_cycle(uint32_t dutyA, uint32_t dutyB)
{
    uint32_t k;

    (dutyA != 0U) ? ledA_on() : ledA_off();
    (dutyB != 0U) ? ledB_on() : ledB_off();

    for (k = 0U; k < PWM_MAX; k++)
    {
        if (k == dutyA) { ledA_off(); }   /* 到达各自占空比就关断 */
        if (k == dutyB) { ledB_off(); }
        delay(UNIT);
    }
}

/* 在给定亮度组合下维持 HOLD 个 PWM 周期 */
static void hold_level(uint32_t dutyA, uint32_t dutyB)
{
    uint32_t h;
    for (h = 0U; h < HOLD; h++)
    {
        pwm_cycle(dutyA, dutyB);
    }
}

int main(void)
{
    uint32_t i;

    /* 打开 GPIOB / GPIOC 时钟 */
    RCC_APB2ENR |= (RCC_IOPBEN | RCC_IOPCEN);

    /* PC13 推挽输出 2MHz (CRH bit[23:20]) */
    GPIOC_CRH &= ~(0xFU << ((LEDA_PIN - 8U) * 4U));
    GPIOC_CRH |=  (0x2U << ((LEDA_PIN - 8U) * 4U));

    /* PB0 推挽输出 2MHz (CRL bit[3:0]) */
    GPIOB_CRL &= ~(0xFU << (LEDB_PIN * 4U));
    GPIOB_CRL |=  (0x2U << (LEDB_PIN * 4U));

    ledA_off();
    ledB_off();

    for (;;)
    {
        /* A 渐亮 / B 渐暗 (反相: B 取反向索引) */
        for (i = 0U; i < LUT_LEN; i++)
        {
            hold_level(gamma_lut[i], gamma_lut[LUT_LEN - 1U - i]);
        }
        /* A 渐暗 / B 渐亮 */
        for (i = LUT_LEN; i > 0U; i--)
        {
            hold_level(gamma_lut[i - 1U], gamma_lut[LUT_LEN - i]);
        }
    }
}
