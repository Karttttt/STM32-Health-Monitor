#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>

// ---------------------------------------------------------
// 函数声明
// ---------------------------------------------------------

// 串口1底层硬件初始化 (参数为波特率，建议115200)
void USART1_Init(uint32_t bound);

// 基础字符串发送函数
void USART1_SendString(char *str);

// 跨平台网页端通信协议封包与发送任务
void Task_SendDataToWeb(float temp, uint8_t hr, uint8_t spo2, short ax, short ay, short az, uint8_t fall_flag, uint8_t finger_on);

#endif