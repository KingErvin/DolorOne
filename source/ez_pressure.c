/*比例法设置*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../include/ez_dolorone.h"
#include "../include/ez_pressure.h"
#include "../include/ez_pump.h"

struct ez_pressure_control ezpressure;

 /*
 * @description			: 读取指定文件内容
 * @param - filename 	: 要读取的文件路径
 * @param - str 		: 读取到的文件字符串
 * @return 				: 0 成功;其他 失败
 */
static int adc_file_data_read(char *filename, char *str)
{
	int ret = 0;
	FILE *data_stream;

    data_stream = fopen(filename, "r"); /* 只读打开 */
    if(data_stream == NULL) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	ret = fscanf(data_stream, "%s", str);
    if(!ret) {
        printf("file read error!\r\n");
    } else if(ret == EOF) {
        /* 读到文件末尾的话将文件指针重新调整到文件头 */
        fseek(data_stream, 0, SEEK_SET);  
    }
	fclose(data_stream);	/* 关闭文件 */	
	return 0;
}

/*获取数据*/
void pressure_data_get(struct ez_pressure_control *press){
    char ascii_number[64];

    adc_file_data_read("/sys/bus/iio/devices/iio:device0/in_voltage1_raw" , ascii_number);
    press->sensor.adc_value_raw = atoi(ascii_number);
    adc_file_data_read("/sys/bus/iio/devices/iio:device0/in_voltage_scale" , ascii_number);
    press->sensor.adc_scale = atof(ascii_number);
    press->sensor.voltage_value = (int)(press->sensor.adc_scale * press->sensor.adc_value_raw);
}

 /*
 * @description			: 写入指定文件内容
 * @param - filename 	: 要写的文件路径
 * @param - str 		: 写到的文件字符串
 * @return 				: 0 成功;其他 失败
 */
// static int dac_file_data_write(char *filename, char *str){
// 	int ret = 0;
// 	FILE *data_stream;

//     data_stream = fopen(filename, "r+"); /* 读写打开 */
//     if(data_stream == NULL) {
// 		printf("can't open file %s\r\n", filename);
// 		return -1;
// 	}

// 	ret = fputs(str , data_stream); /*写数据到文件*/
//     if(!ret) {
//         printf("file read error!\r\n");
//     } 
// 	fclose(data_stream);	/* 关闭文件 */	
// 	return 0;
// }

static int dac_file_data_write(char *filename, char *str){
	int ret = 0;
	FILE *data_stream;

    data_stream = fopen(filename, "r+"); /* 读写打开 */
    if(data_stream == NULL) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	ret = fputs(str , data_stream); /*写数据到文件*/
    if(!ret) {
        printf("file read error!\r\n");
    } 
	fclose(data_stream);	/* 关闭文件 */	
	return 0;
}

void pressure_valve_set(struct ez_pressure_control *press){
    char str[32]={0};

    press->valve.dac_value_raw = (int)(press->valve.voltage_value/press->valve.dac_scale);
    sprintf(str,"%d",press->valve.dac_value_raw);
    // dac_file_data_write("/sys/bus/iio/devices/iio:device0/",str);   //修改路径及文件
}

void pressure_data_handle(struct ez_pressure_control *press){
    press->valve.voltage_value = ezhmi.value[PARAMETER_INTENSITY] / UNIT_INTENSITY_MAX * PRESSURE_BAR_LIMIT / PRESSURE_BAR_MAX * VREF_POWER;
    pressure_valve_set(press);
}

