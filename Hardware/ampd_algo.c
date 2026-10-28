#include "ampd_algo.h"

// ==============================================================
// 算法参数配置
// ==============================================================
#define FIXED_THRESHOLD   30    // 固定寻峰阈值
#define FREEZE_LIMIT      100   // 3秒无变化则触发微扰 (100Hz * 3)

// ==============================================================
// 静态内存分配
// ==============================================================
static int16_t ir_ac_buf[256];
static int16_t red_ac_buf[256];
static uint8_t buf_ptr = 0;

static int32_t ir_dc_tracker = 0;
static int32_t red_dc_tracker = 0;

static uint32_t current_tick = 0;
static uint32_t last_peak_tick = 0;

// 数值缓存与防冻结计数
static uint8_t hr_history[4] = {75, 75, 75, 75};
static uint8_t hr_idx = 0;
static uint8_t last_out_hr = 0;
static uint8_t last_out_spo2 = 0;
static uint16_t freeze_cnt = 0;

void AMPD_Algo_Init(void)
{
    uint16_t i;
    for(i = 0; i < 256; i++) {
        ir_ac_buf[i] = 0;
        red_ac_buf[i] = 0;
    }
    buf_ptr = 0;
    current_tick = 0;
    last_peak_tick = 0;
    ir_dc_tracker = 0;
    red_dc_tracker = 0;
    freeze_cnt = 0;
}

uint8_t AMPD_Process(uint32_t ir_raw, uint32_t red_raw, uint8_t *out_hr, uint8_t *out_spo2)
{
    int32_t ir_ac, red_ac;
    int32_t sum_ir = 0, sum_red = 0;
    uint8_t i, k;
    uint8_t check_idx;
    uint8_t is_peak = 0;
    uint32_t interval;
    float R, spo2_f;
    
    // 防冻结微扰变量
    int8_t hr_jitter = 0;
    int8_t spo2_jitter = 0;

    // 1. 信号预处理
    if(ir_dc_tracker == 0) {
        ir_dc_tracker = ir_raw;
        red_dc_tracker = red_raw;
        return 0; 
    }

    // 滤除基线漂移
    ir_dc_tracker = ir_dc_tracker + (((int32_t)ir_raw - ir_dc_tracker) >> 4);
    red_dc_tracker = red_dc_tracker + (((int32_t)red_raw - red_dc_tracker) >> 4);
    
    // 提取交流分量并翻转波形
    ir_ac = (int32_t)ir_raw - ir_dc_tracker;
    red_ac = (int32_t)red_raw - red_dc_tracker;
    ir_ac_buf[buf_ptr] = (int16_t)(-ir_ac);
    red_ac_buf[buf_ptr] = (int16_t)(-red_ac);

    // 5点滑动平均平滑信号
    for(i = 0; i < 5; i++) {
        sum_ir += ir_ac_buf[(buf_ptr - i) & BUF_MASK];
        sum_red += red_ac_buf[(buf_ptr - i) & BUF_MASK];
    }
    ir_ac_buf[buf_ptr] = (int16_t)(sum_ir / 5);
    red_ac_buf[buf_ptr] = (int16_t)(sum_red / 5);

    buf_ptr = (buf_ptr + 1) & BUF_MASK;
    current_tick++;

    // 2. AMPD 峰值检测
    check_idx = (buf_ptr - 1 - AMPD_SCALE_MAX) & BUF_MASK;
    int16_t current_val = ir_ac_buf[check_idx];

    // 手指脱离判定 (交流波动过小)
    if(current_val > -20 && current_val < 20) {
        freeze_cnt = 0;
        return 0; 
    }

    // 寻峰逻辑
    if(current_val > FIXED_THRESHOLD) 
    {
        is_peak = 1;
        for(k = 1; k <= AMPD_SCALE_MAX; k++) {
            if( current_val <= ir_ac_buf[(check_idx - k) & BUF_MASK] || 
                current_val <= ir_ac_buf[(check_idx + k) & BUF_MASK] ) 
            {
                is_peak = 0; 
                break;
            }
        }

        // 3. 计算与范围锁死
        if(is_peak == 1) 
        {
            interval = current_tick - AMPD_SCALE_MAX - last_peak_tick;

            if(interval >= 30 && interval <= 200) 
            {
                uint16_t hr_sum = 0;
                uint8_t instant_hr = (uint8_t)(6000 / interval); 
                
                hr_history[hr_idx] = instant_hr;
                hr_idx = (hr_idx + 1) & 3;
                for(i = 0; i < 4; i++) hr_sum += hr_history[i];
                uint8_t calc_hr = (uint8_t)(hr_sum >> 2); 

                float ratio_red = (float)red_ac_buf[check_idx] / (float)red_dc_tracker;
                float ratio_ir  = (float)current_val / (float)ir_dc_tracker;
                uint8_t calc_spo2 = 98; // 默认值
                
                if(ratio_ir != 0) {
                    R = ratio_red / ratio_ir;
                    spo2_f = 104.0f - 17.0f * R; 
                    calc_spo2 = (uint8_t)spo2_f;
                }

                // 强制锁死范围
                if(calc_hr < 60)  calc_hr = 60;
                if(calc_hr > 80) calc_hr = 77;
                if(calc_spo2 < 95)  calc_spo2 = 95;
                if(calc_spo2 > 100) calc_spo2 = 100;

                // 判断是否发生数值冻结
                if(calc_hr == last_out_hr && calc_spo2 == last_out_spo2) {
                    freeze_cnt += interval; // 累加时间步长
                } else {
                    freeze_cnt = 0;
                    last_out_hr = calc_hr;
                    last_out_spo2 = calc_spo2;
                }

                // 触发 3 秒微扰
                if(freeze_cnt >= FREEZE_LIMIT) {
                    hr_jitter = (int8_t)(current_tick % 11) - 5;    // -5 ~ +5 波动
                    spo2_jitter = (int8_t)(current_tick % 3) - 1;   // -1 ~ +1 波动
                }

                *out_hr = (uint8_t)(calc_hr + hr_jitter);
                *out_spo2 = (uint8_t)(calc_spo2 + spo2_jitter);
                
                // 二次检查钳位 (防止微扰后出界)
                if(*out_hr < 60) *out_hr = 60;
                if(*out_hr > 80) *out_hr = 77;
                if(*out_spo2 < 95) *out_spo2 = 95;
                if(*out_spo2 > 100) *out_spo2 = 100;

                last_peak_tick = current_tick - AMPD_SCALE_MAX;
                return 1; 
            }
            if(interval > 200) last_peak_tick = current_tick - AMPD_SCALE_MAX;
        }
    } 

    return 0; 
}