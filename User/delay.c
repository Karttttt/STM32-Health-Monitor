#include "delay.h"

// 全局滴答时钟变量
volatile uint32_t g_system_tick = 0;

// =========================================================
// 延时与时间片基础时钟初始化
// =========================================================
void delay_init(void)
{
    // 配置 SysTick 滴答定时器，使其每 1ms 产生一次中断
    SysTick_Config(SystemCoreClock / 1000);
}

// =========================================================
// 毫秒级延时函数 (无损延时，不关定时器)
// =========================================================
void delay_ms(uint32_t nms)
{
    uint32_t start_tick = g_system_tick;
    // 死等直到时间流逝 nms 毫秒
    while((g_system_tick - start_tick) < nms);
}

// =========================================================
// 微秒级延时函数 (读取硬件寄存器实现)
// =========================================================
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;
    
    // 72MHz下，1us等于72个时钟周期
    ticks = nus * (SystemCoreClock / 1000000); 
    told = SysTick->VAL;
    
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told) tcnt += told - tnow;
            else            tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks) break; 
        }
    }
}

// =========================================================
// 兼容性外壳函数：专门应对大写调用的历史遗留代码
// =========================================================
void DelayMs(uint32_t ms)
{
    delay_ms(ms);
}

void DelayUs(uint32_t us)
{
    delay_us(us);
}