#ifndef __EZ_LCD_BSP_H
#define __EZ_LCD_BSP_H

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define NUMBER_ALIGNMENT NUMBER_MID
#define NUMBER_MID 0
#define NUMBER_LIFT 1
#define NUMBER_RIGHT 2

/**** BMP文件头数据结构 ****/
typedef struct {
    unsigned char type[2];      //文件类型
    unsigned int size;          //文件大小
    unsigned short reserved1;   //保留字段1
    unsigned short reserved2;       //保留字段2
    unsigned int offset;        //到位图数据的偏移量
} __attribute__ ((packed)) bmp_file_header;

/**** 位图信息头数据结构 ****/
typedef struct {
    unsigned int size;          //位图信息头大小
    int width;                  //图像宽度
    int height;                 //图像高度
    unsigned short planes;          //位面数
    unsigned short bpp;         //像素深度 
    unsigned int compression;   //压缩方式
    unsigned int image_size;    //图像大小
    int x_pels_per_meter;       //像素/米
    int y_pels_per_meter;       //像素/米 
    unsigned int clr_used;
    unsigned int clr_omportant;
} __attribute__ ((packed)) bmp_info_header;


unsigned short RGB888toRGB565(unsigned char red, unsigned char green, unsigned char blue);
unsigned int RGB565toRGB888(unsigned int color);
void lcd_draw_point(int x, int y, unsigned int color);
void OLED_ShowChar(unsigned int x,unsigned int y,unsigned int chr,unsigned int size1,unsigned int color);
void OLED_ShowString(unsigned int x,unsigned int y,char *chr,unsigned int size1,unsigned int color);
void OLED_ShowNum(unsigned int x,unsigned int y,unsigned int num,unsigned int len,unsigned int size1,unsigned int color);
void lcd_fill(int start_x, int start_y,int end_x, int end_y,unsigned int color);

int showcut_bmp_image(const char *path , int start_x, int start_y , int end_x , int end_y , int target_x , int target_y);

/**** 静态全局变量 ****/
extern int lcd_width;                       //LCD X分辨率
extern int lcd_height;                      //LCD Y分辨率
extern unsigned char *screen_base;        //映射后的显存基地址
// unsigned char *screen_base = NULL;    //输出显示缓冲区
extern unsigned int lcd_bytes_max;       //LCD总字节数（字节为单位）
#endif