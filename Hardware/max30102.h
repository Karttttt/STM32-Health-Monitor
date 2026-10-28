#ifndef __MAX30102_H
#define __MAX30102_H

#include "stm32f10x.h"

// =======================================================
// 位带操作宏定义 (确保 max30102.c 中的直接赋值语法生效)
// =======================================================
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x02000000+((addr &0x000FFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

#define GPIOB_ODR_Addr    (GPIOB_BASE+12) // 0x40010C0C
#define GPIOB_IDR_Addr    (GPIOB_BASE+8)  // 0x40010C08

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)

// =======================================================
// 硬件引脚映射 (严格对应设计：SCL->PB8, SDA->PB9)
// =======================================================
#define MAX30102_IIC_PORT       GPIOB
#define MAX30102_IIC_SCL_PIN    GPIO_Pin_8
#define MAX30102_IIC_SDA_PIN    GPIO_Pin_9

// 模块INT中断引脚定义 (接PA7, 轮询模式下作悬空处理即可)
#define MAX30102_INT_PIN        GPIO_Pin_7 

// IIC 读写电平状态宏 (给底层 .c 调用)
#define MAX30102_IIC_SCL    PBout(8)
#define MAX30102_IIC_SDA    PBout(9)
#define MAX30102_READ_SDA   PBin(9)

// =======================================================
// MAX30102 寻址与寄存器定义
// =======================================================
#define max30102_WR_address 0xAE
#define I2C_WRITE_ADDR      0xAE
#define I2C_READ_ADDR       0xAF
#define I2C_WR              0x00
#define I2C_RD              0x01

// 状态寄存器
#define REG_INTR_STATUS_1   0x00
#define REG_INTR_STATUS_2   0x01
#define REG_INTR_ENABLE_1   0x02
#define REG_INTR_ENABLE_2   0x03

// FIFO 寄存器
#define REG_FIFO_WR_PTR     0x04
#define REG_OVF_COUNTER     0x05
#define REG_FIFO_RD_PTR     0x06
#define REG_FIFO_DATA       0x07

// 配置寄存器
#define REG_FIFO_CONFIG     0x08
#define REG_MODE_CONFIG     0x09
#define REG_SPO2_CONFIG     0x0A
#define REG_LED1_PA         0x0C
#define REG_LED2_PA         0x0D
#define REG_PILOT_PA        0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12

// =======================================================
// Maxim 美信官方算法依赖宏
// =======================================================
#define BUFFER_SIZE         100   // 数据缓冲区大小
#define MA4_SIZE            4     // 滑动平均滤波窗口大小
#define HAMMING_SIZE        31    // 汉明窗大小

// 求小值宏
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

// =======================================================
// 基础操作与算法核心函数声明
// =======================================================
void MAX30102_Init(void);
void MAX30102_Reset(void);

// 提取FIFO红光与红外光数据
void maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led);

// 核心：计算心率与血氧
void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, int32_t *pn_heart_rate, int8_t *pch_hr_valid);

// 内部寻找波峰函数
void maxim_find_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size, int32_t n_min_height);
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
void maxim_sort_ascend(int32_t *pn_x, int32_t n_size);
void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size);

#endif