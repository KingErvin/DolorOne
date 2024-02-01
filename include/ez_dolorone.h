#ifndef __EZPROJECTAPP_H
#define __EZPROJECTAPP_H

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/*系统版本*/
#define VALUE_HARDWARE "Dolor.One HMI_TM050 V1.0.0 2024.1.4"    //硬件版本
#define VALUE_SOFTWARE "Dolor.One HMI Demo V0.0.1 2024.1.26"   //软件版本
#define VALUE_INSTRUCTION "This is a demo file. Include:UI; Exclude pump output and pressure detection."   //版本说明

/*从驱动中获取的数据序号*/
#define TRANSLATE_MAX 4
#define TRANSLATE_BTN 0
#define TRANSLATE_HMI 1
#define TRANSLATE_SHOCK 2
#define TRANSLATE_PRESSURE 3

/*fd序号*/
#define FD_MAX 5
#define FD_SYS 0
#define FD_MT  1
#define FD_LCD 2
#define FD_PWM 3
#define FD_ADC 4

/*多线程的线程ID序号*/
#define THREAD_MAX 6
#define THREAD_INIT 0
#define THREAD_READ 1
#define THREAD_BTN 2
#define THREAD_HMI 3
#define THREAD_SHOCK 4
#define THREAD_PRESSURE 5

/*运行宏*/
#define FUNCTION_DISABLE 0
#define FUNCTION_ENABLE 1
#define FUNCTION_DISABLE_IDLE 2
#define FUNCTION_ENABLE_IDLE 3
#define FUNCTION_DISABLE_FINISH 4

/*参数编号*/
#define PARAMETER_MAXCOUNT   3
#define PARAMETER_IMPULSES   0
#define PARAMETER_INTENSITY  1
#define PARAMETER_FREQUENCY  2

/*参数量程*/
#define UNIT_IMPULSES_MIN     0
#define UNIT_IMPULSES_MAX     5000
#define UNIT_INTENSITY_MIN    20
#define UNIT_INTENSITY_MAX    100
#define UNIT_FREQUENCY_MIN    1
#define UNIT_FREQUENCY_MAX    20

struct system_timer{
    int peroid_shock;
    int peroid_pressure;

    int time_shock;
    int time_pressure;

};

void system_init(void);
void system_read(void);
void btn_handle(void);
void hmi_handle(void);
void pump_handle(void);


#endif