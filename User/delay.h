#ifndef __DELAY_H
#define __DELAY_H 			   
#include "stm32f10x.h"

// 外部声明全局滴答时钟，供主程序使用
extern volatile uint32_t g_system_tick;

void delay_init(void);

// 小写标准版
void delay_ms(uint32_t nms);
void delay_us(uint32_t nus);

// 大写兼容版 (专门为了兼容你工程里 MAX30102 等历史代码)
void DelayMs(uint32_t ms); 
void DelayUs(uint32_t us); 

#endif