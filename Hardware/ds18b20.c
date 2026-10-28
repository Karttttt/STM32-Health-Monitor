#include "ds18b20.h"
#include "delay.h" // 引入全局无损延时，彻底抛弃原来杀定时器的延时

// 配置 PA0 为推挽输出
static void DS18B20_Mode_Out(void)
{
    GPIOA->CRL &= 0xFFFFFFF0; 
    GPIOA->CRL |= 0x00000003; 
}

// 配置 PA0 为上拉输入
static void DS18B20_Mode_In(void)
{
    GPIOA->CRL &= 0xFFFFFFF0; 
    GPIOA->CRL |= 0x00000008; 
    GPIOA->ODR |= 1 << 0;     
}

// 复位 DS18B20
static void DS18B20_Reset(void)
{
    DS18B20_Mode_Out();
    DS18B20_DQ_OUT_LOW();      
    delay_us(750);             // 全部替换为安全的全局 delay_us
    DS18B20_DQ_OUT_HIGH();     
    delay_us(15);              
}

// 等待 DS18B20 响应
static uint8_t DS18B20_Check(void)
{
    uint8_t retry = 0;
    DS18B20_Mode_In();
    
    while (DS18B20_DQ_IN() && retry < 200) {
        retry++;
        delay_us(1);
    }
    if(retry >= 200) return 1;
    
    retry = 0;
    while (!DS18B20_DQ_IN() && retry < 240) {
        retry++;
        delay_us(1);
    }
    if(retry >= 240) return 1;
    
    return 0; 
}

// 从 DS18B20 读取一个位
static uint8_t DS18B20_Read_Bit(void)
{
    uint8_t data;
    DS18B20_Mode_Out();
    DS18B20_DQ_OUT_LOW();
    delay_us(2);
    DS18B20_DQ_OUT_HIGH();
    DS18B20_Mode_In();
    delay_us(12);
    
    if(DS18B20_DQ_IN()) data = 1;
    else data = 0;
    
    delay_us(50);
    return data;
}

// 从 DS18B20 读取一个字节
static uint8_t DS18B20_Read_Byte(void)
{
    uint8_t i, j, dat = 0;
    for (i = 0; i < 8; i++) {
        j = DS18B20_Read_Bit();
        dat = (j << 7) | (dat >> 1);
    }
    return dat;
}

// 写一个字节到 DS18B20
static void DS18B20_Write_Byte(uint8_t dat)
{
    uint8_t j, testb;
    DS18B20_Mode_Out();
    for (j = 0; j < 8; j++) {
        testb = dat & 0x01;
        dat = dat >> 1;
        if (testb) { 
            DS18B20_DQ_OUT_LOW();
            delay_us(2);
            DS18B20_DQ_OUT_HIGH();
            delay_us(60);
        } else {     
            DS18B20_DQ_OUT_LOW();
            delay_us(60);
            DS18B20_DQ_OUT_HIGH();
            delay_us(2);
        }
    }
}

// 初始化 DS18B20
uint8_t DS18B20_Init(void)
{
    uint8_t check_res;
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_0); 

    DS18B20_Reset();
    check_res = DS18B20_Check();
    
    // 极其重要：初始化时预先下发一次转换指令！
    // 这样主循环第一次进来读的时候，就不会读出 85℃ 的错误初始值了
    DS18B20_Reset();
    DS18B20_Check();
    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0x44); // Convert T
    
    return check_res;
}

// 获取温度值（非阻塞纯后台读取版，坚决消灭 750ms 延时！）
short DS18B20_Get_Temp(void)
{
    uint8_t temp_lsb, temp_msb;
    short temp;
    float temp_float;
    
    // 1. 直接读取【上一次】转换好的温度数值
    DS18B20_Reset();
    DS18B20_Check();
    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0xBE); // Read Scratchpad
    
    temp_lsb = DS18B20_Read_Byte(); // LSB
    temp_msb = DS18B20_Read_Byte(); // MSB
    
    // 2. 立刻下发【下一次】的转换指令，然后直接退出函数！
    // 因为你在 main 函数里是 2000ms 进一次，时间远大于它需要的 750ms，下次进来绝对转好了
    DS18B20_Reset();
    DS18B20_Check();
    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0x44); // Convert T
    
    // 3. 计算并返回本次读到的温度
    temp = (temp_msb << 8) | temp_lsb; 
    temp_float = (float)temp * 0.0625;
    
    return (short)(temp_float * 10.0f);
}