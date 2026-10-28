#include "usart.h"
#include <string.h>

// =========================================================
// 串口1底层硬件初始化
// TX -> PA9, RX -> PA10
// =========================================================
void USART1_Init(uint32_t bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 1. 使能 USART1 和 GPIOA 的硬件时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // 2. 配置 USART1 Tx (PA.9) 为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 配置 USART1 Rx (PA.10) 为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. 配置串口基础工作参数
    USART_InitStructure.USART_BaudRate = bound;                                     // 波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式
    USART_Init(USART1, &USART_InitStructure);

    // 5. 开启串口外设
    USART_Cmd(USART1, ENABLE);
}

// =========================================================
// 基础字符串发送函数
// 采用轮询阻塞方式等待发送移位寄存器清空
// =========================================================
void USART1_SendString(char *str)
{
    while (*str != '\0')
    {
        // 向数据寄存器写入当前字符
        USART_SendData(USART1, (uint8_t)*str);
        
        // 严格等待发送完成标志位 (Transmission Complete) 被硬件置位
        while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
        
        str++;
    }
}

// =========================================================
// 跨平台网页端通信协议封包与发送任务
// 将多维物理特征值转换为 JSON 标准格式以供前端 JavaScript 解析
// =========================================================
void Task_SendDataToWeb(float temp, uint8_t hr, uint8_t spo2, short ax, short ay, short az, uint8_t fall_flag, uint8_t finger_on)
{
    char json_buffer[128];
    memset(json_buffer, 0, sizeof(json_buffer));
    
    // 把三轴加速度(AX,AY,AZ)和手指状态(Finger)一起打包进 JSON
    sprintf(json_buffer, "{\"Temp\":%.1f,\"HR\":%d,\"SpO2\":%d,\"AX\":%d,\"AY\":%d,\"AZ\":%d,\"Fall\":%d,\"Finger\":%d}\n", 
            temp, hr, spo2, ax, ay, az, fall_flag, finger_on);
            
    USART1_SendString(json_buffer);
}

// =========================================================
// 引入 printf 重定向支持 (备用，方便底层调试)
// =========================================================
int fputc(int ch, FILE *f)
{      
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
    USART_SendData(USART1, (uint8_t)ch);   
    return ch;
}