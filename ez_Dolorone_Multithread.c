#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <pthread.h>

#include "include/ez_dolorone.h"
#include "include/ez_lcd_fonts.h"
#include "include/ez_lcd_bsp.h"
#include "include/ez_hmi.h"
#include "include/ez_pump.h"
#include "include/ez_pressure.h"

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名		: ledApp.c
作者	  	: 左忠凯
版本	   	: V1.0
描述	   	: chrdevbase驱测试APP。
其他	   	: 无
使用方法	 ：./ledtest /dev/led  0 关闭LED
		     ./ledtest /dev/led  1 打开LED		
论坛 	   	: www.openedv.com
日志	   	: 初版V1.0 2019/1/30 左忠凯创建
***************************************************************/

int transfer_data[TRANSLATE_MAX];
pthread_t id[THREAD_MAX];

int fd[FD_MAX]; //fd:LED,BEEP,BTN; fd1:LCD; fd2:MT; fd3:PWM; fd4:ADC;
int btn_push = 0, mt_irq = 0 ;

pthread_cond_t cond[THREAD_MAX];
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

struct system_timer sys_timer;
/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(void)
{

    int ret;

    pthread_mutex_init(&mutex_lock, NULL);
    pthread_cond_init(&cond[THREAD_INIT], NULL);
    pthread_cond_init(&cond[THREAD_READ], NULL);
    pthread_cond_init(&cond[THREAD_BTN], NULL);
    pthread_cond_init(&cond[THREAD_HMI], NULL);
    pthread_cond_init(&cond[THREAD_SHOCK], NULL);
    pthread_cond_init(&cond[THREAD_PRESSURE], NULL);

    ret = pthread_create(&id[THREAD_INIT] , NULL , (void*(*)(void*))system_init , NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_INIT");
        return 0;
    }
    ret = pthread_join(id[THREAD_INIT], NULL);
    if (ret != 0) {
        printf("等待线程失败\n");
    }
    ret = pthread_create(&id[THREAD_READ],NULL,(void*(*)(void*))system_read,NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_READ");
        return 0;
    }
    ret = pthread_create(&id[THREAD_BTN],NULL,(void*(*)(void*))btn_handle,NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_BTN");
        return 0;
    }
    ret = pthread_create(&id[THREAD_HMI],NULL,(void*(*)(void*))hmi_handle,NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_HMI");
        return 0;
    }
    ret = pthread_create(&id[THREAD_SHOCK],NULL,(void*(*)(void*))pump_handle,NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_PUMP");
        return 0;
    }
    ret = pthread_create(&id[THREAD_PRESSURE],NULL,(void*(*)(void*))pump_handle,NULL);
    if (ret != 0) {
        printf("线程%s创建失败\n","THREAD_PRESSURE");
        return 0;
    }
    pthread_cond_wait(&cond[THREAD_INIT],&mutex_lock); //阻塞函数，信号永不出现
	// while(1){

    // }

    // // /* 退出 */
    // munmap(screen_base, screen_size);  //取消映射
    // close(fd[FD_LCD]);  //关闭文件
    // exit(EXIT_SUCCESS);    //退出进程

	// ret = close(fd[FD_SYS]); /* 关闭文件 */
	// if(ret < 0){
	// 	printf("file “ezdev” close failed!\r\n");
	// 	return -1;
	// }
	return 0;
}

void system_init(void){

	/*关闭休眠*/
    fd[FD_SYS] = open("/dev/tty1", O_RDWR);
    write(fd[FD_SYS], "\033[9;0]", 8);
    close(fd[FD_SYS]);

	/* 打开led,btn驱动 */
	fd[FD_SYS] = open("/dev/ezdev", O_RDWR);
	if(fd[FD_SYS] < 0){
		printf("file %s open failed!\r\n", "/dev/ezdev");
		exit(EXIT_FAILURE);
	}

    /* 打开mt设备 */
    if (0 > (fd[FD_MT] = open("/dev/input/event2", O_RDWR | O_NONBLOCK))) {
        perror("open event2 error");
        exit(EXIT_FAILURE);
    }

    /*初始化触摸显示*/
    display_mt_init(&ezhmi);
    display_ui_init(&ezhmi);
    display_mt_handle(&ezhmi);
    display_mt_update_sp(&ezhmi);

    /*初始化PWM*/
    pump_pwm_init(&ezpump);


}

void system_read(void){
    int retvalue;
    static int start_flag = 0;
    
    while(1){
        // printf("线程%s运行\n","THREAD_READ");
        if(start_flag){
            retvalue = read(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            btn_push = transfer_data[TRANSLATE_BTN];
            mt_irq = transfer_data[TRANSLATE_HMI];
            sys_timer.time_shock = transfer_data[TRANSLATE_SHOCK];
            sys_timer.time_pressure = transfer_data[TRANSLATE_PRESSURE];
        }
        start_flag = 1;
        if(btn_push){
            btn_push = 0;
            transfer_data[TRANSLATE_BTN] = btn_push;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            pthread_cond_signal(&cond[THREAD_BTN]);
            
        }

        /*读取触摸数据*/
        display_mt_read(fd[FD_MT],&ezhmi); //读取触摸
        if(mt_irq){
            mt_irq = 0;
            transfer_data[1] = mt_irq;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            pthread_cond_signal(&cond[THREAD_HMI]);
            
        }

        /*冲击定时响应 1ms */
        if(sys_timer.time_shock){
            sys_timer.time_shock = 0;
            transfer_data[TRANSLATE_SHOCK] = sys_timer.time_shock;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            pthread_cond_signal(&cond[THREAD_SHOCK]);
        }

        /*ADC定时采集 1000ms */
        if(sys_timer.time_pressure){
            sys_timer.time_pressure = 0;
            transfer_data[TRANSLATE_PRESSURE] = sys_timer.time_pressure;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            pthread_cond_signal(&cond[THREAD_PRESSURE]);
        }
    }
}

void btn_handle(void){

    /* 显示BMP图片 */
    // memset(screen_base, 0xFF, screen_size);
    while(1){
        pthread_cond_wait(&cond[THREAD_BTN],&mutex_lock);
        // printf("线程%s即将运行\n","THREAD_BTN");
        if(ezhmi.ui.demo_id > DEMO_NUMBER - 1){
            ezhmi.ui.demo_id = 0;
        }
        else{
            ezhmi.ui.demo_id ++;
        }
        sprintf(ezhmi.ui.pic_path , "./picture/face%d.bmp" , ezhmi.ui.demo_id + 1);
        showcut_bmp_image((const char*)ezhmi.ui.pic_path,0,0,lcd_width,lcd_height,0,0); //显示图片
        //初始化贴图
        display_mt_init(&ezhmi);   //初始化触摸键值
        display_ui_init(&ezhmi); //初始化UI页面显示
        display_mt_handle(&ezhmi); //处理触摸键值
        display_mt_update_sp(&ezhmi); //刷新图片
        //munmap(screen_base, screen_size);  //取消映射
    }
}

void hmi_handle(void){
    
    /*定时刷新触摸显示*/
    while(1){
        pthread_cond_wait(&cond[THREAD_HMI],&mutex_lock);
        // printf("线程%s即将运行\n","THREAD_MT");
        display_mt_handle(&ezhmi); //处理触摸键值
        display_mt_update_sp(&ezhmi); //刷新单次图片
        display_mt_update_rt(&ezhmi); //刷新动态图片
        hmi_send_to_system(&ezhmi);
    }
}

void pump_handle(void){
    
    while(1){
        pthread_cond_wait(&cond[THREAD_SHOCK],&mutex_lock);
        // printf("线程%s即将运行\n","THREAD_SHOCK");
        pump_pwm_update(&ezpump);
        pump_pwm_handle(&ezpump);
        pump_send_to_system(&ezpump);
    }
}

void pressure_handle(void){
    
    while(1){
        pthread_cond_wait(&cond[THREAD_PRESSURE],&mutex_lock);
        // printf("线程%s即将运行\n","THREAD_SHOCK");
        pressure_data_get(&ezpressure);
        pressure_data_handle(&ezpressure);
    }
}

