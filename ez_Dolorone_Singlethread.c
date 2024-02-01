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

struct system_timer sys_timer;
/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(void)
{
	int fd[FD_MAX]; //fd[FB_SYS]:LED,BEEP,BTN; fb[FB_LCD]:LCD; fb[FB_MT]:MT
    int retvalue;
	int btn_push = 0; 
    int mt_irq;
    int start_flag = 0;

    ezhmi.ui.demo_id = 0 ;

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

	while(1){
        if(start_flag){
            retvalue = read(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            btn_push = transfer_data[TRANSLATE_BTN];
            mt_irq = transfer_data[TRANSLATE_HMI];
            sys_timer.time_shock = transfer_data[TRANSLATE_SHOCK];
            sys_timer.time_pressure = transfer_data[TRANSLATE_PRESSURE];
        }
        start_flag = 1;
        // if(mt_irq)
            //  printf("mt_irq:%d\n",mt_irq);
        //  printf("btn_push:%d\nmt_irq:%d\n",btn_push,mt_irq);
        if(btn_push){
            btn_push = 0;
            transfer_data[0] = btn_push;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            
            /* 显示BMP图片 */
            // memset(screen_base, 0xFF, screen_size);

            if(ezhmi.ui.demo_id + 2 >= DEMO_NUMBER){
                ezhmi.ui.demo_id = 0;
            }
            else{
                ezhmi.ui.demo_id ++;
            }
            sprintf(ezhmi.ui.pic_path , "./picture/face%d.bmp" , ezhmi.ui.demo_id + 1);
            showcut_bmp_image(ezhmi.ui.pic_path,0,0,lcd_width,lcd_height,0,0); //显示图片
            //初始化贴图
            display_mt_init(&ezhmi);   //初始化触摸键值
            display_ui_init(&ezhmi); //初始化UI页面显示
            display_mt_handle(&ezhmi); //处理触摸键值
            display_mt_update_sp(&ezhmi); //刷新图片
            //munmap(screen_base, screen_size);  //取消映射
        }

        /*读取触摸数据*/
        display_mt_read(fd[FD_MT],&ezhmi); //读取触摸
        if(mt_irq){
            mt_irq = 0;
            transfer_data[1] = mt_irq;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));

            display_mt_handle(&ezhmi); //处理触摸键值
            display_mt_update_sp(&ezhmi); //刷新单次图片
            display_mt_update_rt(&ezhmi); //刷新动态图片
            hmi_send_to_system(&ezhmi);     //发送数据给系统
            
        }

        /*冲击定时响应 1ms */
        if(sys_timer.time_shock){
            sys_timer.time_shock = 0;
            transfer_data[TRANSLATE_SHOCK] = sys_timer.time_shock;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));
            
            pump_pwm_update(&ezpump);
            pump_pwm_handle(&ezpump);
            pump_send_to_system(&ezpump);
        }

        if(sys_timer.time_pressure){
            sys_timer.time_pressure = 0;
            transfer_data[TRANSLATE_PRESSURE] = sys_timer.time_pressure;
            write(fd[FD_SYS],transfer_data,sizeof(transfer_data));

            pressure_data_get(&ezpressure);
            pressure_data_handle(&ezpressure);
        }
	}

    
    // /* 退出 */
    // munmap(screen_base, screen_size);  //取消映射
    close(fd[FD_LCD]);  //关闭文件
    exit(EXIT_SUCCESS);    //退出进程

	retvalue = close(fd[FD_SYS]); /* 关闭文件 */
	if(retvalue < 0){
		printf("file %s close failed!\r\n", "ezdev");
		return -1;
	}
	return 0;
}

