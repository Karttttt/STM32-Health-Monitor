#include "mpu6050.h"

// 微秒级延时 (加入 volatile 防优化)
static void MPU_Delay_us(uint32_t us)
{
    volatile uint32_t i; 
    for(i=0; i<us*9; i++); 
}

// 配置 SDA 为开漏输出 (必须是开漏 0x7)
static void MPU_SDA_OUT(void)
{
    GPIOB->CRH &= 0xFFFF0FFF;
    GPIOB->CRH |= 0x00007000; 
}

// 配置 SDA 为浮空输入 (必须是浮空 0x4)
static void MPU_SDA_IN(void)
{
    GPIOB->CRH &= 0xFFFF0FFF;
    GPIOB->CRH |= 0x00004000; 
}

// I2C 起始信号
static void MPU_IIC_Start(void)
{
    MPU_SDA_OUT();
    MPU_SDA_Set();
    MPU_SCL_Set();
    MPU_Delay_us(4);
    MPU_SDA_Clr();
    MPU_Delay_us(4);
    MPU_SCL_Clr();
}

// I2C 停止信号
static void MPU_IIC_Stop(void)
{
    MPU_SDA_OUT();
    MPU_SCL_Clr();
    MPU_SDA_Clr();
    MPU_Delay_us(4);
    MPU_SCL_Set();
    MPU_SDA_Set();
    MPU_Delay_us(4);
}

// I2C 等待应答
static uint8_t MPU_IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    MPU_SDA_IN();
    MPU_SDA_Set(); MPU_Delay_us(1);
    MPU_SCL_Set(); MPU_Delay_us(1);
    while(MPU_SDA_READ())
    {
        ucErrTime++;
        if(ucErrTime > 250)
        {
            MPU_IIC_Stop();
            return 1;
        }
    }
    MPU_SCL_Clr();
    return 0;
}

// I2C 产生应答
static void MPU_IIC_Ack(void)
{
    MPU_SCL_Clr();
    MPU_SDA_OUT();
    MPU_SDA_Clr();
    MPU_Delay_us(2);
    MPU_SCL_Set();
    MPU_Delay_us(2);
    MPU_SCL_Clr();
}

// I2C 不产生应答
static void MPU_IIC_NAck(void)
{
    MPU_SCL_Clr();
    MPU_SDA_OUT();
    MPU_SDA_Set();
    MPU_Delay_us(2);
    MPU_SCL_Set();
    MPU_Delay_us(2);
    MPU_SCL_Clr();
}

// I2C 发送一个字节
static void MPU_IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;
    MPU_SDA_OUT();
    MPU_SCL_Clr();
    for(t=0; t<8; t++)
    {
        if((txd & 0x80) >> 7) MPU_SDA_Set();
        else MPU_SDA_Clr();
        txd <<= 1;
        MPU_SCL_Set();
        MPU_Delay_us(2);
        MPU_SCL_Clr();
        MPU_Delay_us(2);
    }
}

// I2C 读取一个字节
static uint8_t MPU_IIC_Read_Byte(uint8_t ack)
{
    uint8_t i, receive = 0;
    MPU_SDA_IN();
    for(i=0; i<8; i++)
    {
        MPU_SCL_Clr();
        MPU_Delay_us(2);
        MPU_SCL_Set();
        receive <<= 1;
        if(MPU_SDA_READ()) receive++;
        MPU_Delay_us(1);
    }
    if(!ack) MPU_IIC_NAck();
    else MPU_IIC_Ack();
    return receive;
}

// 向 MPU6050 写寄存器
static uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data)
{
    MPU_IIC_Start();
    MPU_IIC_Send_Byte(0xD0); // 器件地址 + 写
    if(MPU_IIC_Wait_Ack()) { MPU_IIC_Stop(); return 1; }
    MPU_IIC_Send_Byte(reg);
    MPU_IIC_Wait_Ack();
    MPU_IIC_Send_Byte(data);
    if(MPU_IIC_Wait_Ack()) { MPU_IIC_Stop(); return 1; }
    MPU_IIC_Stop();
    return 0;
}

// 初始化 MPU6050
uint8_t MPU_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    MPU_Write_Byte(0x6B, 0x80); // 复位 MPU6050
    MPU_Delay_us(100000);
    MPU_Write_Byte(0x6B, 0x00); // 唤醒 MPU6050
    MPU_Write_Byte(0x1B, 0x18); // 陀螺仪量程 +-2000度/s
    MPU_Write_Byte(0x1C, 0x00); // 加速度计量程 +-2g (为了跌倒检测更灵敏)
    MPU_Write_Byte(0x19, 0x07); // 采样率 1KHz
    MPU_Write_Byte(0x1A, 0x06); // 低通滤波
    return 0;
}

// 连续读取寄存器数据
static uint8_t MPU_Read_Len(uint8_t reg, uint8_t len, uint8_t *buf)
{
    MPU_IIC_Start();
    MPU_IIC_Send_Byte(0xD0);
    if(MPU_IIC_Wait_Ack()) { MPU_IIC_Stop(); return 1; }
    MPU_IIC_Send_Byte(reg);
    MPU_IIC_Wait_Ack();
    MPU_IIC_Start();
    MPU_IIC_Send_Byte(0xD1); // 器件地址 + 读
    MPU_IIC_Wait_Ack();
    while(len)
    {
        if(len == 1) *buf = MPU_IIC_Read_Byte(0); // 最后一个不应答
        else *buf = MPU_IIC_Read_Byte(1);         // 否则应答
        len--;
        buf++;
    }
    MPU_IIC_Stop();
    return 0;
}

// 读取三轴加速度 (量程+-2g, 输出范围 -32768 ~ 32767)
void MPU_Read_Accel(short *ax, short *ay, short *az)
{
    uint8_t buf[6];
    MPU_Read_Len(0x3B, 6, buf); // 0x3B 是加速度数据首地址
    *ax = ((uint16_t)buf[0] << 8) | buf[1];
    *ay = ((uint16_t)buf[2] << 8) | buf[3];
    *az = ((uint16_t)buf[4] << 8) | buf[5];
}

// ================== 跌倒检测核心算法 ==================
// 返回值：1 检测到跌倒； 0 未检测到
uint8_t MPU_Check_Fall(void)
{
    short ax, ay, az;
    float Ax, Ay, Az, svm_square;
    
    // 1. 读取原始数据
    MPU_Read_Accel(&ax, &ay, &az);
    
    // 2. 转换为标准重力单位 (g)。量程配置为+-2g时，1g = 16384
    Ax = (float)ax / 16384.0f;
    Ay = (float)ay / 16384.0f;
    Az = (float)az / 16384.0f;
    
    // 3. 计算合加速度平方 (省去开方运算节省算力)
    svm_square = Ax*Ax + Ay*Ay + Az*Az;
    
    // 4. 阈值判断：撞击瞬间 SVM 通常大于 2.0g，其平方就是 4.0
    // 注意：2.0g 是经验值，如果测试发现灵敏度太高(容易误报)，可以调大至 2.5g(即平方为6.25)
    // 如果发现跌倒没反应，可以降低至 1.5g(即平方为2.25)
    if(svm_square > 4.0f)
    {
        return 1; // 发生跌倒撞击
    }
    
    return 0; // 正常
}