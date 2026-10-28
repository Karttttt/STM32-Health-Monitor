#include "stm32f10x.h"
#include "delay.h"      
#include "oled.h"       
#include "ds18b20.h"    
#include "mpu6050.h"    
#include "max30102.h"   
#include "ampd_algo.h"  
#include "buzzer.h"     
#include "usart.h"      // 新增串口驱动头文件以支持网页端通信
#include "stdio.h"

// --- 全局时间滴答计数器与任务定时变量 ---
uint32_t timer_max30102 = 0;
uint32_t timer_mpu6050 = 0;
uint32_t timer_usart = 0;
uint32_t timer_oled = 0;
uint32_t timer_ds18b20 = 0;

int main(void)
{
    // --- 1. 核心状态与显示变量 ---
    char display_buf[22];         
    
    // --- 2. 体温与姿态变量 ---
    short temperature = 0;        
    short ax, ay, az;             
    
    // 【新增】：用于记录上一次的加速度，以计算变化率
    short last_ax = 0, last_ay = 0, last_az = 0;
    uint8_t mpu_first_flag = 1; // 第一次读取标志位

    uint8_t is_falling = 0;       

    // --- 3. 心率血氧采集变量 ---
    uint32_t ir_raw, red_raw;     
    uint8_t hr_val = 0;           
    uint8_t spo2_val = 0;         
    uint8_t finger_on = 0;        

    // =========================================================
    // 底层硬件初始化阶段
    // =========================================================
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init();        // 此函数需配置SysTick产生1ms中断并累加g_system_tick
    USART1_Init(115200); // 初始化串口通信模块
    Buzzer_Init();      
    OLED_Init();
    MPU_Init();         
    MAX30102_Init();  
    AMPD_Algo_Init();   
    
    DS18B20_Init(); 
    OLED_Clear();

    // =========================================================
    // 非阻塞式时间片调度框架 (依托 g_system_tick 全局基准时钟)
    // =========================================================
    while(1)
    {
        // ---------------------------------------------------------
        // 任务 1：高频光电采样与算法提取 (10ms执行一次)
        // ---------------------------------------------------------
        if (g_system_tick - timer_max30102 >= 10) 
        {
            timer_max30102 = g_system_tick; // 清零专属计时器
            
            maxim_max30102_read_fifo(&red_raw, &ir_raw);
            if (AMPD_Process(ir_raw, red_raw, &hr_val, &spo2_val) == 1) 
            {
                finger_on = 1; 
            } 
            else if (ir_raw < 10000) 
            {
                finger_on = 0; 
            }
        }

        // ---------------------------------------------------------
        // 任务 2：高频空间姿态与跌倒校验 (20ms执行一次)
        // ---------------------------------------------------------
        if (g_system_tick - timer_mpu6050 >= 20) 
        {
            timer_mpu6050 = g_system_tick;
            
            MPU_Read_Accel(&ax, &ay, &az);
            
            // 如果是刚开机的第一次读取，先存下初始参考值，跳过计算
            if(mpu_first_flag) 
            {
                last_ax = ax; last_ay = ay; last_az = az;
                mpu_first_flag = 0;
            } 
            else 
            {
                // 1. 计算三轴在 20ms 内的变化量（当前值减去历史值）
                int32_t dx = ax - last_ax;
                int32_t dy = ay - last_ay;
                int32_t dz = az - last_az;
                
                // 2. 将变化量转换为直观的标准重力单位 (g)，量程 +-2g 时 1g = 16384
                float dx_g = (float)dx / 16384.0f;
                float dy_g = (float)dy / 16384.0f;
                float dz_g = (float)dz / 16384.0f;
                
                // 3. 计算三维空间中变化率的平方和
                float delta_square = (dx_g * dx_g) + (dy_g * dy_g) + (dz_g * dz_g);
                
                // 4. 动态阈值判定
                // 此时阈值设定为 0.5f，如果觉得还是太难触发，可以改小（如 0.2f）
                // 如果稍微动一下就报警，可以改大（如 0.8f 或 1.0f）
                if(delta_square > 0.5f) 
                {
                    is_falling = 1;
                    BUZZER_ON(); // 触发声学告警
                }
                
                // 5. 覆盖历史值，为下一次 20ms 后的对比做准备
                last_ax = ax;
                last_ay = ay;
                last_az = az;
            }
        }

        // ---------------------------------------------------------
        // 任务 3：中频串口数据封包与网页端同步 (100ms执行一次)
        // ---------------------------------------------------------
        if (g_system_tick - timer_usart >= 100) 
        {
            timer_usart = g_system_tick;
            
            float temp_f = (float)temperature / 10.0f;
            Task_SendDataToWeb(temp_f, hr_val, spo2_val, ax, ay, az, is_falling, finger_on);
        }

        // ---------------------------------------------------------
        // 任务 4：中频本地 OLED 显存页面重绘 (500ms执行一次)
        // ---------------------------------------------------------
        if (g_system_tick - timer_oled >= 500) 
        {
            timer_oled = g_system_tick;
            
            sprintf(display_buf, "Temp: %d.%d C    ", temperature/10, temperature%10);
            OLED_ShowString(0, 0, display_buf);

            if(finger_on) {
                sprintf(display_buf, "HR  : %3d bpm   ", hr_val);
                OLED_ShowString(0, 1, display_buf);
                sprintf(display_buf, "SpO2: %3d %%     ", spo2_val);
                OLED_ShowString(0, 2, display_buf);
            } else {
                OLED_ShowString(0, 1, "HR  : --- bpm   ");
                OLED_ShowString(0, 2, "SpO2: --- %     ");
            }

            sprintf(display_buf, "AccX: %-6d    ", ax);
            OLED_ShowString(0, 3, display_buf);
            sprintf(display_buf, "AccY: %-6d    ", ay);
            OLED_ShowString(0, 4, display_buf);
            sprintf(display_buf, "AccZ: %-6d    ", az);
            OLED_ShowString(0, 5, display_buf);

            if(is_falling) OLED_ShowString(0, 7, "STATUS: FALL!!! ");
            else           OLED_ShowString(0, 7, "STATUS: SAFE    ");
        }
        
        // ---------------------------------------------------------
        // 任务 5：低频热力学参数抓取与报警状态复位 (2000ms执行一次)
        // ---------------------------------------------------------
        if (g_system_tick - timer_ds18b20 >= 2000) 
        {
            timer_ds18b20 = g_system_tick;
            
            temperature = DS18B20_Get_Temp(); 
            if(is_falling) 
            {
                is_falling = 0;
                BUZZER_OFF(); 
            }
        }
    }
}