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

#include "../include/ez_lcd_fonts.h"
#include "../include/ez_lcd_bsp.h"
#include "../include/ez_hmi.h"
#include "../include/ez_pump.h"
#include "../include/ez_dolorone.h"

struct ez_hmi ezhmi;

/*图片UI参数初始化*/
void display_ui_init(struct ez_hmi *hmi){
    int fd;
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    unsigned int screen_size;

    /* 打开framebuffer设备 */
    if (0 > (fd = open("/dev/fb0", O_RDWR))) {
        perror("open fb0 error");
        exit(EXIT_FAILURE);
    }
    /*初始化变量*/
    ezhmi.ui.demo_id = 0;

    /* 获取显示参数信息 */
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);
    
    screen_size = fb_fix.line_length * fb_var.yres;
    lcd_bytes_max = fb_var.xres * fb_var.yres * 4;
    lcd_width = fb_var.xres;
    lcd_height = fb_var.yres;
    printf("\n<屏幕属性>\n"
            "分辨率:%d*%d\n"
            "像素深度bpp:%d\n"
            "总的字节数：%d\n"
            "像素格式:R<%d> G<%d> B<%d>\r\n\r\n",
            fb_var.xres, fb_var.yres,
            fb_var.bits_per_pixel,
            lcd_bytes_max,
            fb_var.red.length,fb_var.green.length,fb_var.blue.length);

    //申请内存
    screen_base = (unsigned char*)malloc(lcd_bytes_max);
    
    /* 将显示缓冲区映射到进程地址空间 */
    screen_base = (unsigned char*)mmap(NULL, screen_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == (void *)screen_base) {
        perror("mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    sprintf(ezhmi.ui.pic_path , "./picture/face%d.bmp" , ezhmi.ui.demo_id + 1);
    showcut_bmp_image((const char*)ezhmi.ui.pic_path,0,0,lcd_width,lcd_height,0,0); //显示图片

    close(fd);

    hmi ->ui.page_index_cur = 1; //当前页面标志初始化
    hmi ->ui.page_index_last = 1;//上次页面标志初始化

    hmi->power_flag = FUNCTION_DISABLE;
    hmi->impluses_lock_flag = FUNCTION_DISABLE;
    hmi->ui.refresh = FUNCTION_ENABLE;
}

/*计算x坐标对应的结果*/
void display_mt_coordtoval(struct ez_hmi *hmi){
    int i;

    for(i = 0 ; i < PARAMETER_MAXCOUNT ; i ++ ){
        hmi->percent[i] = ((float)hmi->mt.coord_x[i] - PIC_FACE1_MT_IMPULSES_START_X)/PIC_FACE1_MT_WIDTH;
        switch(i){
            case PARAMETER_IMPULSES:
            hmi->value[i] = (unsigned int) ((UNIT_IMPULSES_MAX - UNIT_IMPULSES_MIN) * hmi->percent[i] + UNIT_IMPULSES_MIN + 0.5);
            break;
            case PARAMETER_INTENSITY:
            hmi->value[i] = (unsigned int) ((UNIT_INTENSITY_MAX - UNIT_INTENSITY_MIN) * hmi->percent[i] + UNIT_INTENSITY_MIN + 0.5);
            break;
            case PARAMETER_FREQUENCY:
            hmi->value[i] = (unsigned int) ((UNIT_FREQUENCY_MAX - UNIT_FREQUENCY_MIN) * hmi->percent[i] + UNIT_FREQUENCY_MIN + 0.5);
            break;
            default:break;
        }
    }
}

/*计算数值对应的坐标*/
void display_mt_valtocoord(struct ez_hmi *hmi){
    int i;

    for(i = 0 ; i < PARAMETER_MAXCOUNT ; i ++ ){
        switch(i){
            case PARAMETER_IMPULSES:
            hmi->percent[i] = ((float)(hmi->value[i] - UNIT_IMPULSES_MIN)/(UNIT_IMPULSES_MAX - UNIT_IMPULSES_MIN));
            break;
            case PARAMETER_INTENSITY:
            hmi->percent[i] = ((float)(hmi->value[i] - UNIT_INTENSITY_MIN)/(UNIT_INTENSITY_MAX - UNIT_INTENSITY_MIN));
            break;
            case PARAMETER_FREQUENCY:
            hmi->percent[i] = ((float)(hmi->value[i] - UNIT_FREQUENCY_MIN)/(UNIT_FREQUENCY_MAX - UNIT_FREQUENCY_MIN));
            break;
            default:break;
        }
        hmi->mt.coord_x[i] = (unsigned int)(hmi->percent[i] * PIC_FACE1_MT_WIDTH + PIC_FACE1_MT_IMPULSES_START_X + 0.5);
    }
}

/*图片触摸参数初始化*/
void display_mt_init(struct ez_hmi *hmi){
    switch(hmi->ui.demo_id){
        case DEMO_MAIN_1:
        hmi->mt.coord_x[0] = (PIC_FACE1_MT_IMPULSES_END_X - PIC_FACE1_MT_IMPULSES_START_X)/2 + PIC_FACE1_MT_IMPULSES_START_X;
        hmi->mt.coord_y[0] = PIC_FACE1_MT_IMPULSES_MID_Y;
        hmi->mt.coord_x[1] = (PIC_FACE1_MT_INTENSITY_END_X - PIC_FACE1_MT_INTENSITY_START_X)/2 + PIC_FACE1_MT_INTENSITY_START_X;
        hmi->mt.coord_y[1] = PIC_FACE1_MT_INTENSITY_MID_Y;
        hmi->mt.coord_x[2] = (PIC_FACE1_MT_FREQUENCY_END_X - PIC_FACE1_MT_FREQUENCY_START_X)/2 + PIC_FACE1_MT_FREQUENCY_START_X;
        hmi->mt.coord_y[2] = PIC_FACE1_MT_FREQUENCY_MID_Y;
        break;
        case DEMO_MAIN_2:
        hmi->mt.coord_x[0] = (PIC_FACE2_MT_IMPULSES_END_X - PIC_FACE2_MT_IMPULSES_START_X)/2 + PIC_FACE2_MT_IMPULSES_START_X;
        hmi->mt.coord_y[0] = PIC_FACE2_MT_IMPULSES_MID_Y;
        hmi->mt.coord_x[1] = (PIC_FACE2_MT_INTENSITY_END_X - PIC_FACE2_MT_INTENSITY_START_X)/2 + PIC_FACE2_MT_INTENSITY_START_X;
        hmi->mt.coord_y[1] = PIC_FACE2_MT_INTENSITY_MID_Y;
        hmi->mt.coord_x[2] = (PIC_FACE2_MT_FREQUENCY_END_X - PIC_FACE2_MT_FREQUENCY_START_X)/2 + PIC_FACE2_MT_FREQUENCY_START_X;
        hmi->mt.coord_y[2] = PIC_FACE2_MT_FREQUENCY_MID_Y;
        break;
        case DEMO_MAIN_3:
        hmi->mt.coord_x[0] = (PIC_FACE3_MT_IMPULSES_END_X - PIC_FACE3_MT_IMPULSES_START_X)/2 + PIC_FACE3_MT_IMPULSES_START_X;
        hmi->mt.coord_y[0] = PIC_FACE3_MT_IMPULSES_MID_Y;
        hmi->mt.coord_x[1] = (PIC_FACE3_MT_INTENSITY_END_X - PIC_FACE3_MT_INTENSITY_START_X)/2 + PIC_FACE3_MT_INTENSITY_START_X;
        hmi->mt.coord_y[1] = PIC_FACE3_MT_INTENSITY_MID_Y;
        hmi->mt.coord_x[2] = (PIC_FACE3_MT_FREQUENCY_END_X - PIC_FACE3_MT_FREQUENCY_START_X)/2 + PIC_FACE3_MT_FREQUENCY_START_X;
        hmi->mt.coord_y[2] = PIC_FACE3_MT_FREQUENCY_MID_Y;
        break;
        default:break;
    }
    hmi->mt.init = 1;
    hmi->mt.sync = 0;
    for(int i=0;i<3;i++){
        hmi->mt.coord_flag[i] = 1;
        // hmi->mt.iconold_x[i] = hmi->mt.coord_x[i];
    }
    display_mt_coordtoval(hmi);
}

void display_mt_read(int fd , struct ez_hmi *hmi){
    int temp_event_data;
    int mt_size;
    struct input_event inputevent_mt;

    for (temp_event_data = 0 ;temp_event_data < (int)(sizeof(struct input_event)) ;temp_event_data++ ) {
        // printf("mt_irq1:%d\n",mt_irq);
        /* 触摸循环读取数据 */
        mt_size = read(fd, &inputevent_mt, sizeof(struct input_event));
        /*接收到完整事件上报*/
        if(sizeof(struct input_event) == mt_size){
            mt_size = 0;
            /*录入事件及参数*/
            switch(inputevent_mt.type){
                case EV_SYN:
                    hmi->mt.sync   = (unsigned int)inputevent_mt.type;break;/*同步信号*/
                break;
                case EV_KEY:
                switch(inputevent_mt.code){
                    case BTN_TOUCH:         hmi->mt.state  = (unsigned int)inputevent_mt.value;break;/*触摸状态*/
                    default:break;
                }
                break;
                case EV_ABS:
                switch(inputevent_mt.code){
                    case ABS_MT_TRACKING_ID:hmi->mt.id     = (unsigned int)inputevent_mt.value;break;/*触点编号*/
                    case ABS_MT_POSITION_X: hmi->mt.x      = (unsigned int)inputevent_mt.value;break;/*X坐标*/
                    case ABS_MT_POSITION_Y: hmi->mt.y      = (unsigned int)inputevent_mt.value;break;/*Y坐标*/
                    default:break;
                }
                break;
            }
            //触摸代码打印信息
            // printf("type:%d code:%d value:%d <state:%d id:%d>\n",
            //     inputevent_mt.type, inputevent_mt.code, inputevent_mt.value,hmi->mt.state,hmi->mt.id);
        }
    }
}

/*触摸区域检测*/
void display_mt_handle(struct ez_hmi *hmi){
    int i;
    static int xo = 0;

    /*触摸执行*/
    if(!hmi->mt.sync){
        hmi->mt.sync = 1; /*置位同步信号，由于该信号不会自动重置*/
        hmi->mt.display = FUNCTION_ENABLE;
    }
    if(hmi->mt.display){
        hmi->mt.display = FUNCTION_DISABLE;
        hmi->data_send = FUNCTION_ENABLE; //刷新设置
        /*不同页面处理*/
        if(hmi->mt.id != -1){
            switch(hmi->ui.demo_id){
                case DEMO_MAIN_1:
                /*不同触摸区域分控*/
                if ((hmi->impluses_lock_flag == FUNCTION_DISABLE) //启动时次数禁止响应
                    &&(((hmi->mt.x > PIC_FACE1_MT_IMPULSES_START_X)
                    &&(hmi->mt.x < PIC_FACE1_MT_IMPULSES_END_X))
                    &&((hmi->mt.y > PIC_FACE1_MT_IMPULSES_START_Y)
                    &&(hmi->mt.y < PIC_FACE1_MT_IMPULSES_END_Y)))){
                    // printf("IMPULSES\n");

                    hmi->mt.coord_flag[PARAMETER_IMPULSES] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_IMPULSES] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_IMPULSES] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_IMPULSES] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE1_MT_INTENSITY_START_X)
                    &&(hmi->mt.x < PIC_FACE1_MT_INTENSITY_END_X))
                    &&((hmi->mt.y > PIC_FACE1_MT_INTENSITY_START_Y)
                    &&(hmi->mt.y < PIC_FACE1_MT_INTENSITY_END_Y))){
                    // printf("INTENSITY \n");

                    hmi->mt.coord_flag[PARAMETER_INTENSITY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_INTENSITY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_INTENSITY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_INTENSITY] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE1_MT_FREQUENCY_START_X)
                    &&(hmi->mt.x < PIC_FACE1_MT_FREQUENCY_END_X))
                    &&((hmi->mt.y > PIC_FACE1_MT_FREQUENCY_START_Y)
                    &&(hmi->mt.y < PIC_FACE1_MT_FREQUENCY_END_Y))){
                    // printf("FREQUENCY \n");
                    
                    hmi->mt.coord_flag[PARAMETER_FREQUENCY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_FREQUENCY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_FREQUENCY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_FREQUENCY] = hmi->mt.y;
                }
                break;
                case DEMO_MAIN_2:
                // printf("hmi->mt.id:%d hmi->mt.x:%d hmi->mt.y:%d  ",
                //             hmi->mt.id, hmi->mt.x, hmi->mt.y);
                /*不同触摸区域分控*/
                if (((hmi->mt.x > PIC_FACE2_MT_IMPULSES_START_X + PIC_FACE2_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE2_MT_IMPULSES_END_X - PIC_FACE2_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE2_MT_IMPULSES_START_Y)
                    &&(hmi->mt.y < PIC_FACE2_MT_IMPULSES_END_Y))){
                    // printf("IMPULSES\n");

                    hmi->mt.coord_flag[PARAMETER_IMPULSES] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_IMPULSES] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_IMPULSES] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_IMPULSES] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE2_MT_INTENSITY_START_X + PIC_FACE2_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE2_MT_INTENSITY_END_X - PIC_FACE2_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE2_MT_INTENSITY_START_Y)
                    &&(hmi->mt.y < PIC_FACE2_MT_INTENSITY_END_Y))){
                    // printf("INTENSITY \n");

                    hmi->mt.coord_flag[PARAMETER_INTENSITY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_INTENSITY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_INTENSITY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_INTENSITY] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE2_MT_FREQUENCY_START_X + PIC_FACE2_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE2_MT_FREQUENCY_END_X - PIC_FACE2_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE2_MT_FREQUENCY_START_Y)
                    &&(hmi->mt.y < PIC_FACE2_MT_FREQUENCY_END_Y))){
                    // printf("FREQUENCY \n");
                    
                    hmi->mt.coord_flag[PARAMETER_FREQUENCY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_FREQUENCY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_FREQUENCY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_FREQUENCY] = hmi->mt.y;
                }
                break;
                case DEMO_MAIN_3:
                /*不同触摸区域分控*/
                if (((hmi->mt.x > PIC_FACE3_MT_IMPULSES_START_X + PIC_FACE3_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE3_MT_IMPULSES_END_X - PIC_FACE3_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE3_MT_IMPULSES_START_Y)
                    &&(hmi->mt.y < PIC_FACE3_MT_IMPULSES_END_Y))){
                    // printf("IMPULSES\n");

                    hmi->mt.coord_flag[PARAMETER_IMPULSES] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_IMPULSES] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_IMPULSES] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_IMPULSES] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE3_MT_INTENSITY_START_X + PIC_FACE3_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE3_MT_INTENSITY_END_X - PIC_FACE3_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE3_MT_INTENSITY_START_Y)
                    &&(hmi->mt.y < PIC_FACE3_MT_INTENSITY_END_Y))){
                    // printf("INTENSITY \n");

                    hmi->mt.coord_flag[PARAMETER_INTENSITY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_INTENSITY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_INTENSITY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_INTENSITY] = hmi->mt.y;
                }
                if (((hmi->mt.x > PIC_FACE3_MT_FREQUENCY_START_X + PIC_FACE3_MT_CURSOR_W/2)
                    &&(hmi->mt.x < PIC_FACE3_MT_FREQUENCY_END_X - PIC_FACE3_MT_CURSOR_W/2))
                    &&((hmi->mt.y > PIC_FACE3_MT_FREQUENCY_START_Y)
                    &&(hmi->mt.y < PIC_FACE3_MT_FREQUENCY_END_Y))){
                    // printf("FREQUENCY \n");
                    
                    hmi->mt.coord_flag[PARAMETER_FREQUENCY] = FUNCTION_ENABLE;
                    hmi->mt.coord_id[PARAMETER_FREQUENCY] = hmi->mt.id;
                    hmi->mt.coord_x[PARAMETER_FREQUENCY] = hmi->mt.x;
                    hmi->mt.coord_y[PARAMETER_FREQUENCY] = hmi->mt.y;
                }
                break;
                default:break;
            }
        }
        /*页面按键贴图*/
        switch(hmi->mt.state_index){//按键要求在按下前，触摸处于松开状态，避免滑动误触发
            case 0:
            if(hmi->mt.state){ //判断是否按下
                hmi->mt.state_index ++;
            }
            break;
            case 1:
            if(!hmi->mt.state){ //判断是否抬起
                hmi->mt.state_index = 2;
            }
            break;
            default:break;
        }
        if(hmi->mt.state_index == 2){
            hmi->mt.state_index = 0;
            switch(hmi->ui.page_index_cur){
                case PAGE_MAIN:
                if (((hmi->mt.x > PIC_BUTTON_TARGET_INFO_START_X)
                    &&(hmi->mt.x < PIC_BUTTON_TARGET_INFO_END_X))
                    &&((hmi->mt.y > PIC_BUTTON_TARGET_INFO_START_Y)
                    &&(hmi->mt.y < PIC_BUTTON_TARGET_INFO_END_Y))){
                    
                    hmi->ui.page_index_cur = PAGE_INFO;
                    hmi->ui.refresh = FUNCTION_ENABLE;
                    
                }
                if (((hmi->mt.x > PIC_BUTTON_TARGET_CONFIG_START_X)
                    &&(hmi->mt.x < PIC_BUTTON_TARGET_CONFIG_END_X))
                    &&((hmi->mt.y > PIC_BUTTON_TARGET_CONFIG_START_Y)
                    &&(hmi->mt.y < PIC_BUTTON_TARGET_CONFIG_END_Y))){
                    
                    hmi->ui.page_index_cur = PAGE_CONFIG;
                    hmi->ui.refresh = FUNCTION_ENABLE;
                }
                if (((hmi->mt.x > PIC_BUTTON_TARGET_POWER_START_X)
                    &&(hmi->mt.x < PIC_BUTTON_TARGET_POWER_END_X))
                    &&((hmi->mt.y > PIC_BUTTON_TARGET_POWER_START_Y)
                    &&(hmi->mt.y < PIC_BUTTON_TARGET_POWER_END_Y))){
                    xo ^= 1;
                    if(xo){
                        hmi->power_flag = FUNCTION_ENABLE;
                    }
                    else{
                        hmi->power_flag = FUNCTION_DISABLE;
                        hmi->mt.coord_flag[PARAMETER_IMPULSES] = FUNCTION_ENABLE;
                    }
                    hmi->ui.refresh = FUNCTION_ENABLE;
                }
                break;
                case PAGE_INFO:
                if (((hmi->mt.x > PIC_BUTTON_TARGET_BACK_START_X)
                    &&(hmi->mt.x < PIC_BUTTON_TARGET_BACK_END_X))
                    &&((hmi->mt.y > PIC_BUTTON_TARGET_BACK_START_Y)
                    &&(hmi->mt.y < PIC_BUTTON_TARGET_BACK_END_Y))){
                    
                    hmi->ui.page_index_cur = PAGE_MAIN;
                    showcut_bmp_image(hmi->ui.pic_path,     //显示背景图片切换
                                        0,0,
                                        lcd_width,lcd_height,
                                        0,0); 
                    for(i = 0 ; i < PARAMETER_MAXCOUNT ; i++ ){
                        hmi->mt.coord_flag[i] = 1;
                    }
                    hmi->ui.refresh = FUNCTION_ENABLE;
                }
                break;
                case PAGE_CONFIG:
                if (((hmi->mt.x > PIC_BUTTON_TARGET_BACK_START_X)
                    &&(hmi->mt.x < PIC_BUTTON_TARGET_BACK_END_X))
                    &&((hmi->mt.y > PIC_BUTTON_TARGET_BACK_START_Y)
                    &&(hmi->mt.y < PIC_BUTTON_TARGET_BACK_END_Y))){
                    
                    hmi->ui.page_index_cur = PAGE_MAIN;
                    showcut_bmp_image(hmi->ui.pic_path,
                                        0,0,
                                        lcd_width,lcd_height,
                                        0,0); //显示背景图片切换
                    for(i = 0 ; i < PARAMETER_MAXCOUNT ; i++ ){
                        hmi->mt.coord_flag[i] = 1;
                    }
                    hmi->ui.refresh = FUNCTION_ENABLE;
                }
                break;
                default:break;
            }
        }
    }
    
}

void display_mt_update_sp(struct ez_hmi *hmi){
    char tempstring[128];

    if(hmi->ui.refresh == FUNCTION_ENABLE){
        hmi->ui.refresh = FUNCTION_DISABLE;

        switch(hmi->ui.page_index_cur){
        case PAGE_MAIN:
        /*按键图片刷新*/
        //启动空压机按键
        if((hmi->power_flag == FUNCTION_DISABLE)||(hmi->power_flag == FUNCTION_DISABLE_IDLE)){
            hmi->power_flag = FUNCTION_DISABLE_IDLE;
            printf("停止工作\n");
            hmi->init_flag = 0;
            hmi->impluses_lock_flag = FUNCTION_DISABLE; //释放脉冲锁定
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_POWER_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_WHITE_NUM , PIC_BUTTON_SOURCE_POWER_START_Y , 
                                PIC_BUTTON_SOURCE_POWER_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_WHITE_NUM , PIC_BUTTON_SOURCE_POWER_END_Y , 
                                PIC_BUTTON_TARGET_POWER_START_X , PIC_BUTTON_TARGET_POWER_START_Y );            
        }
        else if((hmi->power_flag == FUNCTION_ENABLE)||(hmi->power_flag == FUNCTION_ENABLE_IDLE)){
            hmi->power_flag = FUNCTION_ENABLE_IDLE;
            hmi->impluses_lock_flag = FUNCTION_ENABLE;
            /*pump参数刷新*/
            ezpump.refresh = FUNCTION_ENABLE;
            printf("开始工作\n");
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_POWER_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_GRAY_NUM, PIC_BUTTON_SOURCE_POWER_START_Y , 
                                PIC_BUTTON_SOURCE_POWER_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_GRAY_NUM , PIC_BUTTON_SOURCE_POWER_END_Y , 
                                PIC_BUTTON_TARGET_POWER_START_X , PIC_BUTTON_TARGET_POWER_START_Y );
        }
        else if(hmi->power_flag == FUNCTION_DISABLE_FINISH){
            hmi->power_flag = FUNCTION_DISABLE_IDLE;
            hmi->impluses_lock_flag = FUNCTION_DISABLE; //释放脉冲锁定
            printf("恢复待机\n");
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_POWER_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_GREEN_NUM, PIC_BUTTON_SOURCE_POWER_START_Y , 
                                PIC_BUTTON_SOURCE_POWER_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * ICON_SOURCE_COLOR_GREEN_NUM, PIC_BUTTON_SOURCE_POWER_END_Y , 
                                PIC_BUTTON_TARGET_POWER_START_X , PIC_BUTTON_TARGET_POWER_START_Y );
            display_mt_valtocoord(hmi);
        }
        //信息页面状态
        if(hmi->error_flag){
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_INFO_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * (PAGE_CONFIG-1), PIC_BUTTON_SOURCE_INFO_START_Y , 
                                PIC_BUTTON_SOURCE_INFO_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH *(PAGE_CONFIG-1) , PIC_BUTTON_SOURCE_INFO_END_Y , 
                                PIC_BUTTON_TARGET_INFO_START_X , PIC_BUTTON_TARGET_INFO_START_Y );            
        }
        else if(hmi->error_flag){
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_INFO_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * (PAGE_INFO-1), PIC_BUTTON_SOURCE_INFO_START_Y , 
                                PIC_BUTTON_SOURCE_INFO_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH *(PAGE_INFO-1) , PIC_BUTTON_SOURCE_INFO_END_Y , 
                                PIC_BUTTON_TARGET_INFO_START_X , PIC_BUTTON_TARGET_INFO_START_Y );            
        }
        else{
            showcut_bmp_image("./picture/icon.bmp", 
                                PIC_BUTTON_SOURCE_INFO_START_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH * (PAGE_MAIN-1), PIC_BUTTON_SOURCE_INFO_START_Y , 
                                PIC_BUTTON_SOURCE_INFO_END_X + PIC_BUTTON_SOURCE_INTERVAL_WIDTH *(PAGE_MAIN-1) , PIC_BUTTON_SOURCE_INFO_END_Y , 
                                PIC_BUTTON_TARGET_INFO_START_X , PIC_BUTTON_TARGET_INFO_START_Y );            

        }
        //设置按键
        showcut_bmp_image("./picture/icon.bmp", 
                            PIC_BUTTON_SOURCE_CONFIG_START_X, PIC_BUTTON_SOURCE_CONFIG_START_Y , 
                            PIC_BUTTON_SOURCE_CONFIG_END_X , PIC_BUTTON_SOURCE_CONFIG_END_Y , 
                            PIC_BUTTON_TARGET_CONFIG_START_X , PIC_BUTTON_TARGET_CONFIG_START_Y );
        
        break;
        case PAGE_INFO:
        showcut_bmp_image("./picture/info.bmp",0,0,lcd_width,lcd_height,0,0); //显示图片
        /*现实信息内容*/
        OLED_ShowString(PIC_INFO_STRING_ROWINDENT,PIC_INFO_STRING_LINEINDENT + PIC_INFO_STRING_LINESPACING * 0 , (char*)information_title , FRONT_INFO , COLOR_WHITE);
        memset(tempstring,'\0',sizeof(tempstring));
        strcpy(tempstring , information_hardware);
        strcat(tempstring , VALUE_HARDWARE);
        OLED_ShowString(PIC_INFO_STRING_ROWINDENT,PIC_INFO_STRING_LINEINDENT + PIC_INFO_STRING_LINESPACING * 1 , tempstring , FRONT_INFO , COLOR_WHITE);    //硬件版本
        memset(tempstring,'\0',sizeof(tempstring));
        strcpy(tempstring , information_software);
        strcat(tempstring , VALUE_SOFTWARE);
        OLED_ShowString(PIC_INFO_STRING_ROWINDENT,PIC_INFO_STRING_LINEINDENT + PIC_INFO_STRING_LINESPACING * 2 , tempstring , FRONT_INFO , COLOR_WHITE);    //软件版本
        memset(tempstring,'\0',sizeof(tempstring));
        strcpy(tempstring , information_instruction);
        strcat(tempstring , VALUE_INSTRUCTION);
        OLED_ShowString(PIC_INFO_STRING_ROWINDENT,PIC_INFO_STRING_LINEINDENT + PIC_INFO_STRING_LINESPACING * 3 , tempstring , FRONT_INFO , COLOR_WHITE);    //软件版本
        //设置按键
        showcut_bmp_image("./picture/icon.bmp", PIC_BUTTON_SOURCE_BACK_START_X, PIC_BUTTON_SOURCE_BACK_START_Y , PIC_BUTTON_SOURCE_BACK_END_X , PIC_BUTTON_SOURCE_BACK_END_Y , PIC_BUTTON_TARGET_BACK_START_X , PIC_BUTTON_TARGET_BACK_START_Y );            

        break;
        case PAGE_CONFIG:
        showcut_bmp_image("./picture/info.bmp",0,0,lcd_width,lcd_height,0,0); //显示图片
        //设置按键
        showcut_bmp_image("./picture/icon.bmp", PIC_BUTTON_SOURCE_BACK_START_X, PIC_BUTTON_SOURCE_BACK_START_Y , PIC_BUTTON_SOURCE_BACK_END_X , PIC_BUTTON_SOURCE_BACK_END_Y , PIC_BUTTON_TARGET_BACK_START_X , PIC_BUTTON_TARGET_BACK_START_Y );            

        break;
        default:break;
        }
    }
}

/*参数的具体计算以及动态贴图显示*/
void display_mt_update_rt(struct ez_hmi *hmi){
    int i;
    int temp_x;
    static int temp_x_old;

    switch(hmi->ui.page_index_cur){
        /*参数设置界面*/
        case PAGE_MAIN:
        switch(hmi->ui.demo_id){
            case DEMO_MAIN_1:
            /*数据更新*/
            if(hmi->mt.coord_flag[PARAMETER_IMPULSES]||hmi->mt.coord_flag[PARAMETER_INTENSITY]||hmi->mt.coord_flag[PARAMETER_FREQUENCY]){
                display_mt_coordtoval(hmi); //赋值变量
                /*现实界面刷新*/
                for(i = 0 ; i < PARAMETER_MAXCOUNT ; i ++ ){
                    if(hmi->mt.coord_flag[i]){
                        hmi->mt.coord_flag[i] = 0;
                        // printf("icon_switch:%d coord_x:%d coord_y:%d\n\n",(unsigned int)i,hmi->mt.coord_x[i],hmi->mt.coord_y[i]);
                        /*进度条显示*/
                        // showcut_bmp_image("./picture/face1_line_full.bmp", 0 , 0 , hmi->mt.coord_x[i] - PIC_FACE1_MT_IMPULSES_START_X , PIC_FACE1_MT_HEIGTH , PIC_FACE1_MT_IMPULSES_START_X , PIC_FACE1_MT_IMPULSES_START_Y + i * PIC_FACE1_MT_STEP_Y );
                        // showcut_bmp_image("./picture/face1_line_empty.bmp", hmi->mt.coord_x[i] - PIC_FACE1_MT_IMPULSES_START_X , 0 , PIC_FACE1_MT_WIDTH , PIC_FACE1_MT_HEIGTH , hmi->mt.coord_x[i] , PIC_FACE1_MT_IMPULSES_START_Y + i * PIC_FACE1_MT_STEP_Y );
                        if(hmi->mt.coord_old_x[i] < PIC_FACE1_MT_IMPULSES_START_X){ //避免坐标为负数
                           hmi->mt.coord_old_x[i] = PIC_FACE1_MT_IMPULSES_START_X ;
                        }
                        if(hmi->mt.coord_old_x[i] < hmi->mt.coord_x[i]){
                            showcut_bmp_image("./picture/face1_line_full.bmp", hmi->mt.coord_old_x[i] - PIC_FACE1_MT_IMPULSES_START_X , 0 , hmi->mt.coord_x[i] - PIC_FACE1_MT_IMPULSES_START_X , PIC_FACE1_MT_HEIGTH , hmi->mt.coord_old_x[i] , PIC_FACE1_MT_IMPULSES_START_Y + i * PIC_FACE1_MT_STEP_Y );
                        }
                        else{
                            showcut_bmp_image("./picture/face1_line_empty.bmp", hmi->mt.coord_x[i] - PIC_FACE1_MT_IMPULSES_START_X , 0 , hmi->mt.coord_old_x[i] - PIC_FACE1_MT_IMPULSES_START_X, PIC_FACE1_MT_HEIGTH , hmi->mt.coord_x[i] , PIC_FACE1_MT_IMPULSES_START_Y + i * PIC_FACE1_MT_STEP_Y );
                        }
                        hmi->mt.coord_old_x[i] = hmi->mt.coord_x[i];

                        lcd_fill(PIC_FACE1_DP_IMPULSES_NUM_START_X , PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*PIC_FACE1_MT_STEP_Y , 190, PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*PIC_FACE1_MT_STEP_Y + 60 , COLOR_BLACK);
                        /*数值显示*/
                        switch(i){
                            case PARAMETER_IMPULSES:
                            OLED_ShowNum(PIC_FACE1_DP_IMPULSES_NUM_START_X,PIC_FACE1_DP_IMPULSES_NUM_START_Y,hmi->value[i],4,FRONT_GLOBAL,COLOR_WHITE);
                            break;
                            case PARAMETER_INTENSITY:
                            OLED_ShowNum(PIC_FACE1_DP_IMPULSES_NUM_START_X,PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*PIC_FACE1_MT_STEP_Y,hmi->value[i],3,FRONT_GLOBAL,COLOR_WHITE);
                            //OLED_ShowChar(PIC_FACE1_DP_IMPULSES_NUM_START_X + FRONT_GLOBAL/2*3,PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*160,'%',FRONT_GLOBAL,COLOR_WHITE);
                            break;
                            case PARAMETER_FREQUENCY:
                            OLED_ShowNum(PIC_FACE1_DP_IMPULSES_NUM_START_X,PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*PIC_FACE1_MT_STEP_Y,hmi->value[i],3,FRONT_GLOBAL,COLOR_WHITE);
                            //OLED_ShowString(PIC_FACE1_DP_IMPULSES_NUM_START_X + FRONT_GLOBAL/2*3 , PIC_FACE1_DP_IMPULSES_NUM_START_Y + i*160 , (char*)unit_hz , FRONT_GLOBAL , COLOR_WHITE);
                            break;
                            default:break;
                        }
                    } 
                } 
            }
            /*实时刷新剩余次数*/
            if(((hmi->power_flag == FUNCTION_ENABLE)||(hmi->power_flag == FUNCTION_ENABLE_IDLE))&&(hmi->mt.coord_old_x[PARAMETER_IMPULSES] != hmi->mt.coord_x[PARAMETER_IMPULSES])){
                display_mt_coordtoval(hmi);
                showcut_bmp_image("./picture/face1_line_lock.bmp", 0 , 0 , hmi->mt.coord_x[PARAMETER_IMPULSES] - PIC_FACE1_MT_IMPULSES_START_X , PIC_FACE1_MT_HEIGTH , PIC_FACE1_MT_IMPULSES_START_X , PIC_FACE1_MT_IMPULSES_START_Y + PARAMETER_IMPULSES * PIC_FACE1_MT_STEP_Y );
                showcut_bmp_image("./picture/face1_line_empty.bmp", hmi->mt.coord_x[PARAMETER_IMPULSES] - PIC_FACE1_MT_IMPULSES_START_X , 0 , PIC_FACE1_MT_WIDTH , PIC_FACE1_MT_HEIGTH , hmi->mt.coord_x[PARAMETER_IMPULSES] , PIC_FACE1_MT_IMPULSES_START_Y + PARAMETER_IMPULSES * PIC_FACE1_MT_STEP_Y );

                lcd_fill(PIC_FACE1_DP_IMPULSES_NUM_START_X , PIC_FACE1_DP_IMPULSES_NUM_START_Y + PARAMETER_IMPULSES*PIC_FACE1_MT_STEP_Y , 190, PIC_FACE1_DP_IMPULSES_NUM_START_Y + PARAMETER_IMPULSES*PIC_FACE1_MT_STEP_Y + 60 , COLOR_BLACK);
                OLED_ShowNum(PIC_FACE1_DP_IMPULSES_NUM_START_X,PIC_FACE1_DP_IMPULSES_NUM_START_Y,hmi->value[PARAMETER_IMPULSES],4,FRONT_GLOBAL,COLOR_WHITE);  
                
                hmi->mt.coord_old_x[PARAMETER_IMPULSES] = hmi->mt.coord_x[PARAMETER_IMPULSES];
            }
            break;
            case DEMO_MAIN_2:
            /*数据更新*/
            if(hmi->mt.coord_flag[PARAMETER_IMPULSES]||hmi->mt.coord_flag[PARAMETER_INTENSITY]||hmi->mt.coord_flag[PARAMETER_FREQUENCY]){
                display_mt_coordtoval(hmi);
                for(i = 0 ; i < 3 ; i++ ){
                    if(hmi->mt.coord_flag[i]){
                        hmi->mt.coord_flag[i] = 0;
                        // printf("icon_switch:%d coord_x:%d coord_y:%d\n\n",(unsigned int)i,hmi->mt.coord_x[i],hmi->mt.coord_y[i]);
                        showcut_bmp_image("./picture/face2_line_empty.bmp",0 , 0 , PIC_FACE2_MT_WIDTH , PIC_FACE2_MT_HEIGTH , PIC_FACE2_MT_IMPULSES_START_X,PIC_FACE2_MT_IMPULSES_START_Y + i*165 -i);
                        lcd_fill(hmi->mt.coord_x[i] - PIC_FACE2_MT_CURSOR_W/2 , PIC_FACE2_MT_IMPULSES_MID_Y + i*PIC_FACE2_MT_STEP_Y - PIC_FACE2_MT_CURSOR_H/2 , hmi->mt.coord_x[i] + PIC_FACE2_MT_CURSOR_W/2 , PIC_FACE2_MT_IMPULSES_MID_Y + i*165 + PIC_FACE2_MT_CURSOR_H/2 , COLOR_WHITE);
                        lcd_fill(PIC_FACE2_DP_IMPULSES_NUM_START_X , PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y , 240, PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y + 50 , COLOR_BLACK);
                        hmi->percent[i] = (hmi->mt.coord_x[i] - PIC_FACE2_MT_IMPULSES_START_X)/PIC_FACE2_MT_WIDTH;
                        switch(i){
                            case 0:
                            OLED_ShowNum(PIC_FACE2_DP_IMPULSES_NUM_START_X,PIC_FACE2_DP_IMPULSES_NUM_START_Y,hmi->value[i],4,FRONT_GLOBAL,COLOR_WHITE);
                            break;
                            case 1:
                            OLED_ShowNum(PIC_FACE2_DP_IMPULSES_NUM_START_X,PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y,hmi->value[i],3,FRONT_GLOBAL,COLOR_WHITE);
                            OLED_ShowChar(PIC_FACE2_DP_IMPULSES_NUM_START_X + FRONT_GLOBAL/2*3,PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y,'%',FRONT_GLOBAL,COLOR_WHITE);
                            break;
                            case 2:
                            OLED_ShowNum(PIC_FACE2_DP_IMPULSES_NUM_START_X,PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y,hmi->value[i],3,FRONT_GLOBAL,COLOR_WHITE);
                            OLED_ShowString(PIC_FACE2_DP_IMPULSES_NUM_START_X + FRONT_GLOBAL/2*3 , PIC_FACE2_DP_IMPULSES_NUM_START_Y + i*PIC_FACE2_MT_STEP_Y , (char*)unit_hz , FRONT_GLOBAL , COLOR_WHITE);
                            break;
                            default:break;
                        }

                    } 
                }
            }
            break;
            case DEMO_MAIN_3:
            /*数据更新*/
            if(hmi->mt.coord_flag[PARAMETER_IMPULSES]||hmi->mt.coord_flag[PARAMETER_INTENSITY]||hmi->mt.coord_flag[PARAMETER_FREQUENCY]){
                display_mt_coordtoval(hmi);
                for(i = 0 ; i < 3 ; i++ ){
                    if(hmi->mt.coord_flag[i]){
                        hmi->mt.coord_flag[i] = 0;
                        // printf("icon_switch:%d coord_x:%d coord_y:%d\n\n",(unsigned int)i,hmi->mt.coord_x[i],hmi->mt.coord_y[i]);
                        temp_x = 200;
                        for(int j = 0;j<5;j++){
                            if(hmi->mt.coord_x[i] > (unsigned int)(200 + PIC_FACE3_MT_STEP_X * j + PIC_FACE3_MT_STEP_X/2)){
                                temp_x = 200 + PIC_FACE3_MT_STEP_X * (j+1);
                            }
                        }
                        if(temp_x != temp_x_old){
                            hmi->mt.coord_x[i] = temp_x;
                            temp_x_old = temp_x;
                            showcut_bmp_image("./picture/face3.bmp", PIC_FACE3_MT_IMPULSES_START_X , PIC_FACE3_MT_IMPULSES_START_Y + i*PIC_FACE3_MT_STEP_Y , PIC_FACE3_MT_IMPULSES_END_X , PIC_FACE3_MT_IMPULSES_END_Y + i*PIC_FACE3_MT_STEP_Y , PIC_FACE3_MT_IMPULSES_START_X , PIC_FACE3_MT_IMPULSES_START_Y + i*PIC_FACE3_MT_STEP_Y );
                            lcd_fill(200 - PIC_FACE3_MT_CURSOR_W/2 , PIC_FACE3_MT_IMPULSES_MID_Y + i*PIC_FACE3_MT_STEP_Y - PIC_FACE3_MT_CURSOR_H/2 , hmi->mt.coord_x[i] + PIC_FACE3_MT_CURSOR_W/2 , PIC_FACE3_MT_IMPULSES_MID_Y + i*PIC_FACE3_MT_STEP_Y + PIC_FACE3_MT_CURSOR_H/2 , COLOR_WHITE);
                        }
                        // switch(i){
                        //     case 0:
                        //     hmi->mt.value[i] = (unsigned int) (UNIT_IMPULSES * (hmi->mt.coord_x[i] - PIC_FACE3_MT_IMPULSES_START_X)/(PIC_FACE3_MT_IMPULSES_END_X - PIC_FACE3_MT_IMPULSES_START_X));
                        //     break;
                        //     case 1:
                        //     hmi->mt.value[i] = (unsigned int) (UNIT_INTENSITY * (hmi->mt.coord_x[i] - PIC_FACE3_MT_INTENSITY_START_X)/(PIC_FACE3_MT_INTENSITY_END_X - PIC_FACE3_MT_INTENSITY_START_X));
                        //     break;
                        //     case 2:
                        //     hmi->mt.value[i] = (unsigned int) (UNIT_FREQUENCY * (hmi->mt.coord_x[i] - PIC_FACE3_MT_FREQUENCY_START_X)/(PIC_FACE3_MT_FREQUENCY_END_X - PIC_FACE3_MT_FREQUENCY_START_X));
                        //     break;
                        //     default:break;
                        // }

                    } 
                }
            }
            break;
            default:break;
        }
        
        break;
        /*信息界面*/
        case PAGE_INFO:
        
        break;
        /*设置界面*/
        case PAGE_CONFIG:
        break;
    }
}

void Work_Init(struct ez_hmi *hmi){
    if(!hmi->init_flag){
        hmi->init_flag ++;

    }

}

void hmi_send_to_system(struct ez_hmi *hmi){
    int i;

    if(hmi->data_send == FUNCTION_ENABLE){
        hmi->data_send = FUNCTION_DISABLE;

        /*将设置参数发送给PUMP*/
        for(i = 0 ; i < PARAMETER_MAXCOUNT ; i ++ ){
            ezpump.value[i] = hmi->value[i];
        }
        if((hmi->power_flag == FUNCTION_ENABLE)||(hmi->power_flag == FUNCTION_ENABLE_IDLE)){
            ezpump.power_flag = FUNCTION_ENABLE;
        }
        else{
            ezpump.power_flag = FUNCTION_DISABLE;
            ezpump.shock_left = ezpump.value[PARAMETER_IMPULSES];
        }

        ezpump.refresh = FUNCTION_ENABLE;
    }
}