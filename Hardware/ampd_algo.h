#ifndef __AMPD_ALGO_H
#define __AMPD_ALGO_H

#include "stm32f10x.h"

// 采样率
#define SAMPLE_RATE         100 
// 缓冲区大小 (必须是 2的幂次方，如 256，方便位与运算代替取模，速度极快)
#define BUF_MASK            255 
// AMPD 多尺度扫描的最大尺度 (对应约 150ms 的半波宽度)
#define AMPD_SCALE_MAX      15  

// 初始化
void AMPD_Algo_Init(void);

// 核心解算函数 (每次传入1组最新数据，实时返回结果)
// 返回值：1代表检测到一次有效心跳并更新，0代表未检测到或数据无效
uint8_t AMPD_Process(uint32_t ir_raw, uint32_t red_raw, uint8_t *out_hr, uint8_t *out_spo2);

#endif