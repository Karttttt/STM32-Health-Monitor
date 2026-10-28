#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

// 定义硬件引脚 (PA1)
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_Pin_1

// 低电平触发逻辑宏定义
#define BUZZER_ON()  GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN)  // 输出低电平，蜂鸣器响
#define BUZZER_OFF() GPIO_SetBits(BUZZER_PORT, BUZZER_PIN)    // 输出高电平，蜂鸣器停

void Buzzer_Init(void);

#endif