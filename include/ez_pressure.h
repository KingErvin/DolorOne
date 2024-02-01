#ifndef __EZ_PRESSURE_H
#define __EZ_PRESSURE_H

#include "ez_dolorone.h"

#define PRESSURE_BAR_MAX      10    //最大的气压
#define PRESSURE_BAR_LIMIT      4    //最大的气压
#define DAC_AMPLIFIER_MULTIPLE 2    //放大器倍数
#define VREF_POWER 3.3    //基准电源

/* 气压读取结构体*/
struct pressure_sensor{
    int adc_value_raw;
    float adc_scale;
    float voltage_value;
};

struct valve_control{
    int dac_value_raw;
    float dac_scale;
    float voltage_value;
};

struct ez_pressure_control{
    struct pressure_sensor sensor;
    struct valve_control valve;
};

void pressure_data_get(struct ez_pressure_control *press);
void pressure_data_handle(struct ez_pressure_control *press);
void pressure_valve_set(struct ez_pressure_control *press);

extern struct ez_pressure_control ezpressure;

#endif /* __EZ_PRESSURE_H */
