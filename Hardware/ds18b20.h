#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f10x.h"

// ---------------- 定义硬件引脚 (PA0) ----------------
#define DS18B20_DQ_OUT_HIGH() GPIO_SetBits(GPIOA, GPIO_Pin_0)
#define DS18B20_DQ_OUT_LOW()  GPIO_ResetBits(GPIOA, GPIO_Pin_0)
#define DS18B20_DQ_IN()       GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)

// ---------------- 函数声明 ----------------
uint8_t DS18B20_Init(void);
short DS18B20_Get_Temp(void);

#endif