#include "oled.h"
#include "oledfont.h"

// 简单的微秒级延时函数（适用于软件I2C时序控制）
static void OLED_Delay(void)
{
    uint8_t i = 10;
    while(i--);
}

// I2C 起始信号
void IIC_Start(void)
{
    OLED_SDA_Set();
    OLED_SCL_Set();
    OLED_Delay();
    OLED_SDA_Clr();
    OLED_Delay();
    OLED_SCL_Clr();
}

// I2C 停止信号
void IIC_Stop(void)
{
    OLED_SCL_Clr();
    OLED_SDA_Clr();
    OLED_Delay();
    OLED_SCL_Set();
    OLED_SDA_Set();
    OLED_Delay();
}

// I2C 等待应答 (简化处理，直接等待不报错)
void IIC_Wait_Ack(void)
{
    OLED_SCL_Set();
    OLED_Delay();
    OLED_SCL_Clr();
    OLED_Delay();
}

// I2C 发送一个字节
void IIC_Send_Byte(uint8_t txd)
{
    uint8_t i;
    for(i=0; i<8; i++)
    {
        if(txd & 0x80) OLED_SDA_Set();
        else           OLED_SDA_Clr();
        OLED_Delay();
        OLED_SCL_Set();
        OLED_Delay();
        OLED_SCL_Clr();
        txd <<= 1;
    }
    IIC_Wait_Ack();
}

// 向 OLED 写命令
void OLED_Write_Cmd(uint8_t cmd)
{
    IIC_Start();
    IIC_Send_Byte(0x78); // OLED 默认 I2C 地址 (0x3C << 1)
    IIC_Send_Byte(0x00); // 0x00 表示发送命令
    IIC_Send_Byte(cmd);
    IIC_Stop();
}

// 向 OLED 写数据
void OLED_Write_Data(uint8_t data)
{
    IIC_Start();
    IIC_Send_Byte(0x78);
    IIC_Send_Byte(0x40); // 0x40 表示发送数据
    IIC_Send_Byte(data);
    IIC_Stop();
}

// 设置光标坐标
void OLED_Set_Pos(uint8_t x, uint8_t y) 
{ 
    OLED_Write_Cmd(0xb0 + y);
    OLED_Write_Cmd(((x & 0xf0) >> 4) | 0x10);
    OLED_Write_Cmd((x & 0x0f) | 0x01);
} 

// 清屏
void OLED_Clear(void)  
{  
    uint8_t i,n;        
    for(i=0; i<8; i++)  
    {  
        OLED_Write_Cmd(0xb0 + i);
        OLED_Write_Cmd(0x00);
        OLED_Write_Cmd(0x10);
        for(n=0; n<128; n++) OLED_Write_Data(0);
    } 
}

// 显示单个字符 (使用 6x8 字体)
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr)
{      
    uint8_t c = 0, i = 0;
    c = chr - ' '; // 获取字符的偏移量
    if(x > 122) { x = 0; y++; } // 换行
    OLED_Set_Pos(x, y);
    for(i=0; i<6; i++)
    {
        OLED_Write_Data(F6x8[c][i]);
    }
}

// 显示字符串
void OLED_ShowString(uint8_t x, uint8_t y, char *chr)
{
    uint8_t j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j]);
        x += 6; // 6x8 字体的宽度为 6
        if(x > 120) { x = 0; y++; }
        j++;
    }
}

// GPIO 和 OLED 屏幕初始化
void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 开启 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. 初始化 PB6, PB7 为开漏输出 (I2C需要开漏模式并接上拉电阻，多数OLED模块自带上拉)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    OLED_SCL_Set();
    OLED_SDA_Set();

    // 3. 简单的软件延时等待屏幕上电稳定
    uint32_t delay = 100000;
    while(delay--);

    // 4. OLED 初始化序列 (SSD1306 推荐配置)
    OLED_Write_Cmd(0xAE); // 关闭显示
    OLED_Write_Cmd(0x20); // 设置寻址模式
    OLED_Write_Cmd(0x10); // 页寻址模式
    OLED_Write_Cmd(0xB0); // 设置页地址
    OLED_Write_Cmd(0xC8); // COM 输出扫描方向 (翻转)
    OLED_Write_Cmd(0x00); // 低列地址
    OLED_Write_Cmd(0x10); // 高列地址
    OLED_Write_Cmd(0x40); // 开始行地址
    OLED_Write_Cmd(0x81); // 对比度控制
    OLED_Write_Cmd(0xFF); // 亮度最大
    OLED_Write_Cmd(0xA1); // 段重映射 (翻转)
    OLED_Write_Cmd(0xA6); // 正常显示 (非反相)
    OLED_Write_Cmd(0xA8); // 多路复用率
    OLED_Write_Cmd(0x3F); // 1/64 duty
    OLED_Write_Cmd(0xA4); // 全局显示开启
    OLED_Write_Cmd(0xD3); // 设置显示偏移
    OLED_Write_Cmd(0x00); // 偏移为0
    OLED_Write_Cmd(0xD5); // 设置振荡器频率
    OLED_Write_Cmd(0xF0);
    OLED_Write_Cmd(0xD9); // 设置预充电周期
    OLED_Write_Cmd(0x22);
    OLED_Write_Cmd(0xDA); // COM 引脚硬件配置
    OLED_Write_Cmd(0x12);
    OLED_Write_Cmd(0xDB); // VCOMH
    OLED_Write_Cmd(0x20);
    OLED_Write_Cmd(0x8D); // 电荷泵配置
    OLED_Write_Cmd(0x14); // 开启电荷泵
    OLED_Write_Cmd(0xAF); // 开启显示
    
    OLED_Clear();
}