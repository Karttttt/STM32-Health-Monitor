#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

// ---------------- 定义硬件引脚 ----------------
// I2C SCL -> PB6
#define OLED_SCL_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_6)
#define OLED_SCL_Set() GPIO_SetBits(GPIOB, GPIO_Pin_6)

// I2C SDA -> PB7
#define OLED_SDA_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_7)
#define OLED_SDA_Set() GPIO_SetBits(GPIOB, GPIO_Pin_7)

// ---------------- 函数声明 ----------------
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowString(uint8_t x, uint8_t y, char *chr);

#endif