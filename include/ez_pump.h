#ifndef __EZ_PUMP_H
#define __EZ_PUMP_H

#include "ez_dolorone.h"
#include "ez_hmi.h"

/*PWM通道
*无刷电机相位U:与驱动版PWM信号复用
*无刷电机相位V
*无刷电机相位W
*修改前需要先修改设备树
*/
#define PWM_CH_U 2 
#define PWM_CH_V 3
#define PWM_CH_W 4

#define PWM_PERIOD 100000 //PWM输出周期
#define CORE_PERIOD 1000 //系统节拍周期

/**** PWM功能结构体 ****/
struct ez_pwm{
    int period;
    float duty;

};
/**** PUMP功能结构体 ****/
struct ez_pump{
    int data_send;       //数据发送开关
    int power_flag;     //启动开关
    int init_flag;      //初始化开关
    int refresh;        //刷新标志
    int warning_flag;   //报警标志,位检测。
    int error_flag;     //错误标志,位检测。

    unsigned int value[PARAMETER_MAXCOUNT]; //设置的参数数值保存
    struct ez_pwm extern_pwm;//PWM调速
    int pause_flag;     //暂停开关
    int shock_set;  //设置脉冲
    int shock_left;  //剩余脉冲
    int shock_frequency;//频率
    int shock_count_max;    //最大计数
    int shock_count_cur;    //计数
    int pressure_output;    //压力

};

int pump_pwm_set(const char *attr, int val);
void pump_pwm_init(struct ez_pump *pump);
void pump_pwm_handle(struct ez_pump *pump );
void pump_pwm_config(struct ez_pump *pump );
void pump_pwm_update(struct ez_pump *pump );
void pump_send_to_system(struct ez_pump *pump);

extern struct ez_pump ezpump;
#endif /* ez_pump_bsp */
