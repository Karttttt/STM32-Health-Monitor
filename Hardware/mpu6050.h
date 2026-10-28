#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32f10x.h"

// ---------------- 定义硬件引脚 (PB10, PB11) ----------------
#define MPU_SCL_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#define MPU_SCL_Set() GPIO_SetBits(GPIOB, GPIO_Pin_10)

#define MPU_SDA_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_11)
#define MPU_SDA_Set() GPIO_SetBits(GPIOB, GPIO_Pin_11)

#define MPU_SDA_READ() GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)

// ---------------- 函数声明 ----------------
uint8_t MPU_Init(void);
void MPU_Read_Accel(short *ax, short *ay, short *az);
uint8_t MPU_Check_Fall(void);

#endif