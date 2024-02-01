#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../include/ez_dolorone.h"
#include "../include/ez_pump.h"
#include "../include/ez_hmi.h"

static char pwm_path[100];
struct ez_pump ezpump;

/*PWM初始化*/
void pump_pwm_init(struct ez_pump *pump){
    /* 初始化pwm路径 */
    sprintf(pwm_path, "/sys/class/pwm/pwmchip%d/pwm0", PWM_CH_U);
    if(access(pwm_path, F_OK)) {//如果pwm0目录不存在, 则导出

        char temp[100];
        int fd;
        printf("pwm0 creat\n");
        sprintf(temp, "/sys/class/pwm/pwmchip%d/export", PWM_CH_U);
        if(0 > (fd = open(temp, O_WRONLY))) {
            perror("open error");
            exit(-1);
        }

        if(1 != write(fd, "0", 1)) {//导出pwm
            perror("write error");
            close(fd);
            exit(-1);
        }

        close(fd);  //关闭文件
    }

    /*设置PWM周期*/
    pump->extern_pwm.period = PWM_PERIOD;
    pump_pwm_set("period", pump->extern_pwm.period);

    /*复位参数*/
    pump->pause_flag = FUNCTION_DISABLE;
    pump->init_flag = FUNCTION_DISABLE;
    // if(pump->power_flag){
    //     pump->impulses_left = pump->value[PARAMETER_INTENSITY];
    //     pump->impulses_frequency = pump->value[PARAMETER_FREQUENCY];
    //     pump->pressure_output = pump->value[PARAMETER_INTENSITY];
    // }
}

/*PWM参数更新*/
int pump_pwm_set(const char *attr, int val){
    char file_path[100];
    int fd;
    char tempchar[10] = {0};
    // printf("%s/%s > %s\n", pwm_path, attr, val);
    sprintf(file_path, "%s/%s", pwm_path, attr);
    if (0 > (fd = open(file_path, O_WRONLY))) {
        perror("open error");
        return fd;
    }

    sprintf(tempchar,"%d",val);
    write(fd, tempchar, strlen(tempchar));
    // len = strlen(val);
    // if (len != write(fd, val, len)) {
    //     perror("write error");
    //     close(fd);
    //     return -1;
    // }

    close(fd);  //关闭文件
    return 0;
}

void pump_pwm_update(struct ez_pump *pump){
    
    
    /* 导入PUMP设置 */
    if((pump->power_flag == FUNCTION_ENABLE)||(pump->power_flag == FUNCTION_ENABLE_IDLE)){
        /*初始化*/
        if(pump->init_flag == FUNCTION_DISABLE){
            pump->init_flag = FUNCTION_ENABLE;

            pump->shock_set = pump->value[PARAMETER_IMPULSES];
            pump->shock_left = pump->shock_set;
            /*设置参数*/
            pump_pwm_config(pump);
            /*启动PUMP*/
            pump_pwm_set("enable", FUNCTION_ENABLE);
            
        }
        else{
            pump->shock_count_cur ++ ;
            if(pump->shock_count_cur >= pump->shock_count_max){
                pump->shock_count_cur = 0;
                pump->data_send = FUNCTION_ENABLE; //每次脉冲刷新数据用于显示
                if(!pump->shock_left){
                    pump->power_flag = FUNCTION_DISABLE_FINISH;
                    printf("工作完成\n");
                    pump_pwm_init(pump);
                }
                else{
                    printf("冲击剩余%d\n",pump->shock_left);//实际此处替换为电磁阀开关
                    pump->shock_left --;
                }
            }
            /*如果中途有变化*/
            pump_pwm_config(pump);
        }
        
    }
    else{
        pump_pwm_set("enable", FUNCTION_DISABLE);
    }
}

void pump_pwm_config(struct ez_pump *pump){
    int i;

    /*数据读取*/
    if(pump->refresh == FUNCTION_ENABLE){
        pump->refresh = FUNCTION_DISABLE;
        for(i = 0 ; i < PARAMETER_MAXCOUNT ; i ++ ){
            pump->value[i] = ezhmi.value[i];
        }
        /*频率设置*/
        pump->shock_frequency = pump->value[PARAMETER_FREQUENCY];
        pump->shock_count_max = CORE_PERIOD / pump->shock_frequency;
        /*压力输出设置*/
        pump->pressure_output = pump->value[PARAMETER_INTENSITY];

        /*外部PWM占空比调节*/
        pump->extern_pwm.duty = PWM_PERIOD * (pump->value[PARAMETER_INTENSITY] - UNIT_INTENSITY_MIN)/(UNIT_INTENSITY_MAX - UNIT_INTENSITY_MIN);
        pump_pwm_set("duty_cycle", pump->extern_pwm.duty);
    }
}


/*压力读取*/
int pump_pwm_pressure_get(void){

    return 0;
}

/*泵PWM运算处理函数*/
void pump_pwm_handle(struct ez_pump *pump ){
    
    pump_pwm_pressure_get();
    if(pump->power_flag){
        
    }

}


void pump_send_to_system(struct ez_pump *pump){

    if(pump->data_send){
        pump->data_send = FUNCTION_DISABLE;
        ezhmi.power_flag = pump->power_flag;
        
        ezhmi.value[PARAMETER_IMPULSES] = pump->shock_left;
        ezhmi.mt.coord_x[PARAMETER_IMPULSES] = (unsigned int)((float)(ezhmi.value[PARAMETER_IMPULSES] - UNIT_IMPULSES_MIN) / (UNIT_IMPULSES_MAX - UNIT_IMPULSES_MIN) * PIC_FACE1_MT_WIDTH + PIC_FACE1_MT_IMPULSES_START_X + 0.5);

        /*停止时刷新按键状态*/
        if(pump->power_flag == FUNCTION_DISABLE_FINISH){

            ezhmi.ui.refresh = FUNCTION_ENABLE;
            ezhmi.value[PARAMETER_IMPULSES] = pump->shock_set;
        }
    }
    
}