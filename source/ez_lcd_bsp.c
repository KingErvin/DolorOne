#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#include "../include/ez_lcd_bsp.h"
#include "../include/ez_lcd_fonts.h"

/**** 静态全局变量 ****/
int lcd_width;                       //LCD X分辨率
int lcd_height;                      //LCD Y分辨率
unsigned char *screen_base = NULL;        //映射后的显存基地址
// unsigned char *screen_base = NULL;    //输出显示缓冲区
unsigned int lcd_bytes_max;       //LCD总字节数（字节为单位）

unsigned short RGB888toRGB565(unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned short B = (blue >> 3) & 0x001F;
    unsigned short G = ((green >> 2) << 5) & 0x07E0;
    unsigned short R = ((red >> 3) << 11) & 0xF800;
    // printf("b:%d,g:%d,r:%d\r\n",blue,green,red);
    // printf("B:%d,G:%d,R:%d\r\n",(blue >> 3),(green >> 2),(red >> 3));
    return (unsigned short) (R | G | B);
}

unsigned int RGB565toRGB888(unsigned int color)
{
    unsigned int B = (color & 0x001F) << 3 ;
    unsigned int G = (color & 0x07E0) << 5 ;
    unsigned int R = (color & 0xF800) << 8 ;
    // printf("b:%d,g:%d,r:%d\r\n",blue,green,red);
    // printf("B:%d,G:%d,R:%d\r\n",(blue >> 3),(green >> 2),(red >> 3));
    return (R | G | B);
}
/********************************************************************
 * 函数名称： lcd_draw_point
 * 功能描述： 
 * //在指定位置显示一个字符,包括部分字符
 * //x:0~800
 * //y:0~480
 * //size1:选择字体
 * //color:颜色
 * 输入参数： x, y, color
 * 返 回 值： 无
 ********************************************************************/

void OLED_ShowChar(unsigned int x,unsigned int y,unsigned int chr,unsigned int size1,unsigned int color)
{
  unsigned int i,m,temp,size2,chr1;
  unsigned int x0=x,y0=y;
  if(size1==8)size2=6;
  else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
  chr1=chr-' ';  //计算偏移后的值
  for(i=0;i<size2;i++)
  {
    if(size1==32)
    {temp=font_Transbold_Neue_Euro_Bold_1632[chr1][i];} //调用1632字体
    else if(size1==48)
    {temp=font_Transbold_Neue_Euro_Bold_2448[chr1][i];} //调用2448字体
    else if(size1==64)
    {temp=font_Transbold_Neue_Euro_Bold_3264[chr1][i];} //调用3264字体
    else if(size1==16)
    {temp=font_Song_0816[chr1][i];} //调用0816字体
    else if(size1==24)
    {temp=font_Song_1224[chr1][i];} //调用1224字体

    else return;
    for(m=0;m<8;m++)
    {
      if(temp&0x01)
        lcd_draw_point(x,y,color);
      temp>>=1;
      y++;
    }
    x++;
    if((size1!=8)&&((x-x0)==size1/2))
    {x=x0;y0=y0+8;}
    y=y0;
  }
}
//显示字符串
//x,y:起点坐标  
//size1:字体大小 
//*chr:字符串起始地址 
//mode:0,反色显示;1,正常显示
void OLED_ShowString(unsigned int x,unsigned int y,char *chr,unsigned int size1,unsigned int color)
{
    printf("%c",*chr);
    while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
    {
        OLED_ShowChar(x,y,*chr,size1,color);
        if(size1==8)x+=6;
        else x+=size1/2;
        chr++;
    }
}

//m^n
unsigned int OLED_Pow(unsigned int m,unsigned int n)
{
  unsigned int result=1;
  while(n--)
  {
    result*=m;
  }
  return result;
}

//显示数字
//x,y :起点坐标
//num :要显示的数字
//len :数字的位数
//size:字体大小
//mode:0,反色显示;1,正常显示
void OLED_ShowNum(unsigned int x,unsigned int y,unsigned int num,unsigned int len,unsigned int size1,unsigned int color)
{
#if NUMBER_ALIGNMENT == NUMBER_MID
  unsigned int t,temp,m = 0,wide,em = 0,em_count = 0,xstart = 0,len_count = 0;
  
  if(size1==8)m=2;
  wide = size1/2 + m; //一个数字的宽度
  //OLED_ShowPicture(x,y,wide*len,size1,(unsigned int*)Pic_Clear,color);
  for(t=0;t<len;t++)
  {
    temp=(num/OLED_Pow(10,len-t-1))%10;
    //当前为最高位，且高位为0
    if(!em){  
      if(temp){//判断首位是否为0
        em++;
        xstart = em_count*wide/2 + x; //计算实际起始点
        OLED_ShowChar(xstart+wide*len_count++,y,temp+'0',size1,color);
      }
      else{
        em_count ++;//计算实际位数
        if(em_count == len){ //如果该数实际为0
          xstart = (em_count-1)*wide/2 + x; //计算实际起始点
          OLED_ShowChar(xstart,y,temp+'0',size1,color);
        }
      }
    }
    else
      OLED_ShowChar(xstart+wide*len_count++,y,temp+'0',size1,color);
  }
#else
    u8 t,temp,m=0;
    if(size1==8)m=2;
    for(t=0;t<len;t++)
    {
      temp=(num/OLED_Pow(10,len-t-1))%10;
      if(temp==0)
      {
        OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,color);
      }
      else 
      {
        OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,color);
      }
    }
#endif
}

//显示数字
//x,y :起点坐标
//num :要显示的数字
//len :数字的整数位数
//len2:数字的小数位数
//format：数字的对其方式
//size:字体大小
//color:字体颜色
void OLED_ShowNumFloat(int x,int y,float num,int len,int len2,int size1,int format, int color)
{
  int t,temp,m=0;
  float tempf32;
  
  if(!len)return;
  
  int wide,em = 0,em_count = 0,xstart = 0,len_count = 0;
  
  if(size1==8)m=2;
  wide = size1/2 + m; //一个数字的宽度
  // OLED_ShowPicture(x,y,wide*(len+len2 + 1),size1,(int*)Pic_Clear,color);
  for(t=0;t<(len + len2);t++){
    if((t == len)&&(len2)){//有小数
      OLED_ShowChar(xstart+wide*len_count++,y,'.',size1,color);
    }
    
    tempf32 = num*OLED_Pow(10,len2); //计算总显示数，转化为整数
    temp=((unsigned int)tempf32/OLED_Pow(10,len + len2-t-1))%10; //算出每一位数
    //当前为最高位，且高位为0
    if(t<len){//整数阶段
      if(!em){  //当前是否为首位
        if(temp){//当前首位非0
          em++;
          if(format == NUMBER_MID){//居中显示  
            if(len2)
              xstart = (em_count - 2 + len2)*wide/2 + x; //计算实际起始点
            else
              xstart = em_count*wide/2 + x; //计算实际起始点
          }
          else if(format == NUMBER_RIGHT){
            if(len2)
              xstart = (em_count - 2 + len2)*wide + x; //计算实际起始点
            else
              xstart = em_count*wide + x; //计算实际起始点
          }
          else
            xstart = x;
          OLED_ShowChar(xstart+wide*len_count++,y,temp+'0',size1,color);
        }
        else{//当前首位为0
          em_count ++;//计算实际位数
          if(em_count == len){ //如果该数实际为0
            if(format == NUMBER_MID){//居中显示  
              if(len2)  //有小数则加上小数点和整数0
                xstart = (em_count - 3 + len2)*wide/2 + x; //计算实际起始点
              else      //没小数只加整数0
                xstart = (em_count - 1 + len2)*wide/2 + x; //计算实际起始点
            }
            else if(format == NUMBER_RIGHT){
              if(len2)  //有小数则加上小数点和整数0
                xstart = (em_count - 3 + len2)*wide + x; //计算实际起始点
              else      //没小数只加整数0
                xstart = (em_count - 1 + len2)*wide + x; //计算实际起始点
            }
            else
              xstart = x;
            OLED_ShowChar(xstart + wide*len_count++,y,temp+'0',size1,color);
          }
        }
      }
      else{
        OLED_ShowChar(xstart+wide*len_count++,y,temp+'0',size1,color);
      }
    }
    else{
      OLED_ShowChar(xstart+wide*len_count++,y,temp+'0',size1,color);
    }
    
  }
}

/********************************************************************
 * 函数名称： lcd_draw_point
 * 功能描述： 打点
 * 输入参数： x, y, color
 * 返 回 值： 无
 ********************************************************************/
void lcd_draw_point(int x, int y, unsigned int color)
{
    // unsigned short rgb565_color = argb8888_to_rgb565(color);//得到RGB565颜色值

    /* 对传入参数的校验 */
    if (x >= lcd_width)
        x = lcd_width - 1;
    if (y >= lcd_height)
        y = lcd_height - 1;

    /* 填充颜色 */
    screen_base[(y * lcd_width + x)*4+0] = color & 0xff;
    screen_base[(y * lcd_width + x)*4+1] = (color >> 8) & 0xff;
    screen_base[(y * lcd_width + x)*4+2] = (color >> 16) & 0xff;
    screen_base[(y * lcd_width + x)*4+3] = 0;
}

/********************************************************************
 * 函数名称： lcd_draw_line
 * 功能描述： 画线（水平或垂直线）
 * 输入参数： x, y, dir, length, color
 * 返 回 值： 无
 ********************************************************************/
void lcd_draw_line(int x, int y, int dir,
            unsigned int length, unsigned int color)
{
    // unsigned short rgb565_color = argb8888_to_rgb565(color);//得到RGB565颜色值
    int end;
    unsigned long temp;

    /* 对传入参数的校验 */
    if (x >= lcd_width)
        x = lcd_width - 1;
    if (y >= lcd_height)
        y = lcd_height - 1;

    /* 填充颜色 */
    temp = y * lcd_width + x;//定位到起点
    if (dir) {  //水平线
        end = x + length - 1;
        if (end >= lcd_width)
            end = lcd_width - 1;

        for ( ; x <= end; x++, temp++){
            screen_base[temp*4+0] = color & 0xff;    
            screen_base[temp*4+1] = (color >> 8) & 0xff;
            screen_base[temp*4+2] = (color >> 16) & 0xff;
            screen_base[temp*4+3] = 0;
        }
    }
    else {  //垂直线
        end = y + length - 1;
        if (end >= lcd_height)
            end = lcd_height - 1;

        for ( ; y <= end; y++, temp += lcd_width){
            screen_base[temp*4+0] = color & 0xff;    
            screen_base[temp*4+1] = (color >> 8) & 0xff;
            screen_base[temp*4+2] = (color >> 16) & 0xff;
            screen_base[temp*4+3] = 0;
        }
    }
}

/********************************************************************
 * 函数名称： lcd_draw_rectangle
 * 功能描述： 画矩形
 * 输入参数： start_x, end_x, start_y, end_y, color
 * 返 回 值： 无
 ********************************************************************/
void lcd_draw_rectangle(unsigned int start_x, unsigned int end_x,
            unsigned int start_y, unsigned int end_y,
            unsigned int color)
{
    int x_len = end_x - start_x + 1;
    int y_len = end_y - start_y - 1;

    lcd_draw_line(start_x, start_y, 1, x_len, color);//上边
    lcd_draw_line(start_x, end_y, 1, x_len, color); //下边
    lcd_draw_line(start_x, start_y + 1, 0, y_len, color);//左边
    lcd_draw_line(end_x, start_y + 1, 0, y_len, color);//右边
}

/********************************************************************
 * 函数名称： lcd_fill
 * 功能描述： 将一个矩形区域填充为参数color所指定的颜色
 * 输入参数： start_x, end_x, start_y, end_y, color
 * 返 回 值： 无
 ********************************************************************/
void lcd_fill(int start_x, int start_y,
              int end_x, int end_y,
              unsigned int color)
{
    // unsigned short rgb565_color = argb8888_to_rgb565(color);//得到RGB565颜色值
    int temp;
    int y;
    int x;

    /* 对传入参数的校验 */
    if (end_x >= lcd_width)
        end_x = lcd_width - 1;
    if (end_y >= lcd_height)
        end_y = lcd_height - 1;

    /* 填充颜色 */
    temp = end_x - start_x;
    // printf("flag\n");
    for ( y = start_y; y <= end_y; y++) {
        for (x = start_x; x <= end_x; x++){
            screen_base[(y*lcd_width+x)*4+0] = color & 0xff;
            screen_base[(y*lcd_width+x)*4+1] = (color >> 8) & 0xff;
            screen_base[(y*lcd_width+x)*4+2] = (color >> 16) & 0xff;
            screen_base[(y*lcd_width+x)*4+3] = 0;
        }
    }
    // memcpy(screen_base, screen_base, lcd_bytes_max);//刷入LCD显存
}

//x,y:圆心坐标
//r:圆的半径
void OLED_DrawCircle(int x,int y,int r,int color)
{
  int a, b,num;
  a = 0;
  b = r;
  while(2 * b * b >= r * r)      
  {
    lcd_draw_point(x + a, y - b,color);
    lcd_draw_point(x - a, y - b,color);
    lcd_draw_point(x - a, y + b,color);
    lcd_draw_point(x + a, y + b,color);
    
    lcd_draw_point(x + b, y + a,color);
    lcd_draw_point(x + b, y - a,color);
    lcd_draw_point(x - b, y - a,color);
    lcd_draw_point(x - b, y + a,color);
    
    a++;
    num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
    if(num > 0)
    {
      b--;
      a--;
    }
  }
}

/********************************************************************
 * 函数名称： show_bmp_image
 * 功能描述： 在LCD上显示指定的BMP图片
 * 输入参数： 文件路径
 * 返 回 值： 成功返回0, 失败返回-1
 ********************************************************************/
static int show_bmp_image(const char *path , int target_x , int target_y)
{
    bmp_file_header file_h;
    bmp_info_header info_h;
    unsigned char *pic_buf_get = NULL;    //原图缓冲区
    
    unsigned int pic_get_bytes;   //BMP图像一帧的字节的大小
    // unsigned int frame_cut_bytes;   //BMP图像实际裁剪后一帧的字节的大小
    // unsigned int frame_set_bytes;   //BMP图像实际裁剪后转为LCD显示深度一帧的字节的大小
    unsigned int min_h, min_w;
    int fd1 = -1;
    unsigned int color565,color888;
    int i,j;

    /* 打开文件 */ 
    if (0 > (fd1 = open(path, O_RDONLY))) {
        perror("open error");
        return -1;
    }

    /* 读取BMP文件头 */
    if (sizeof(bmp_file_header) !=
        read(fd1, &file_h, sizeof(bmp_file_header))) {
        perror("read error");
        close(fd1);
        return -1;
    }

    if (0 != memcmp(file_h.type, "BM", 2)) {
        fprintf(stderr, "it's not a BMP file\n");
        close(fd1);
        return -1;
    }

    /* 读取位图信息头 */
    if (sizeof(bmp_info_header) !=
        read(fd1, &info_h, sizeof(bmp_info_header))) {
        perror("read error");
        close(fd1);
        return -1;
    }

    /* 打印信息 */
    // printf("\n<图片属性>\n文件大小: %d\n"
    //      "位图数据的偏移量: %d\n"
    //      "位图信息头大小: %d\n"
    //      "图像分辨率: %d*%d\n"
    //      "像素深度: %d\n", file_h.size, file_h.offset,
    //      info_h.size, info_h.width, info_h.height,
    //      info_h.bpp);

    /* 将文件读写位置移动到图像数据开始处 */
    if (-1 == lseek(fd1, file_h.offset, SEEK_SET)) {
        perror("lseek error");
        close(fd1);
        return -1;
    }

    /* 申请一个buf、暂存bmp图像的一行数据 */
    pic_get_bytes = info_h.width * info_h.height * info_h.bpp / 8;
    pic_buf_get = (unsigned char*)malloc(pic_get_bytes);
    if (NULL == pic_buf_get){
        fprintf(stderr, "malloc error\n");
        close(fd1);
        return -1;
    }
    // memset(screen_base, 0xFF, lcd_bytes_max);
    // memcpy(screen_base, screen_base, lcd_bytes_max);//刷入LCD显存

    /**** 计算图片显示是否需要裁剪 ****/
    if (0 < info_h.height){//倒向位图
        if (lcd_width > (info_h.width + target_x)){
            min_w = info_h.width+target_x;
        }
        else{
            min_w = lcd_width;
        }
        if (lcd_height > (info_h.height + target_y)){
            min_h = info_h.height+target_y;
            //screen_base += width * (info_h.height - 1); //定位到....不知怎么描述 懂的人自然懂!
        }
        else {
            min_h = lcd_height;
            lseek(fd1, (info_h.height - lcd_height) * info_h.width, SEEK_CUR);
            //screen_base += width * (height - 1);    //定位到屏幕左下角位置
        }

        

        /**** 显示图片 ****/
        // printf("target_x:%d target_y:%d\r\n",target_x,target_y);
        // // printf("target_y:%d\r\n",target_y);
        // printf("pic_get_bytes:%d\r\n",pic_get_bytes);
        read(fd1, pic_buf_get, pic_get_bytes); //读取出图像数据
        for (j = 0; j < info_h.height; j++){
            for (i = 0; i < info_h.width; i++){
                if(info_h.bpp == 32){
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 0] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*4];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 1] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*4 +1];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 2] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*4 +2];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 3] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*4 +3];
                    //printf("%d %d %d\r\n",line_buf_set[(j*min_w+i)*3],line_buf_set[(j*min_w+i)*3+1],line_buf_set[(j*min_w+i)*3+2]);
                }
                else if(info_h.bpp == 24){
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 0] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*3];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 1] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*3 +1];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 2] = pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*3 +2];
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 3] = 0;
                    //printf("%d %d %d\r\n",line_buf_set[(j*min_w+i)*3],line_buf_set[(j*min_w+i)*3+1],line_buf_set[(j*min_w+i)*3+2]);
                }
                if(info_h.bpp == 16){
                    color565 = (pic_buf_get[((info_h.height-1 - j) * info_h.width + i)*2 + 1]<<8) + pic_buf_get[((info_h.height-1 - j)*info_h.width+i)*2];
                    color888 = RGB565toRGB888(color565);
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 0] = color888 & 0xff;
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 1] = (color888 >> 8) & 0xff;
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 2] = (color888 >> 16) & 0xff;
                    screen_base[((target_y + j) * lcd_width + i + target_x) * 4 + 3] = 0;
                    //printf("%d %d %d %d %d\r\n",color565,color888,screen_base[(j*min_w+i)*4],screen_base[(j*min_w+i)*4+1],screen_base[(j*min_w+i)*4+2]);
                    
                }
            }
        }
        // memcpy(screen_base, screen_base, lcd_bytes_max);//刷入LCD显存
    }
    // else {  //正向位图
    //     int temp = 0 - info_h.height;   //负数转成正数
    //     if (temp > height)
    //         min_h = height;
    //     else
    //         min_h = temp;

    //     for (j = min_h; j > 0; screen_base -= width, j--) {
    //         read(fd1, line_buf_get, line_bytes); //读取出图像数据
    //         if(info_h.bpp == 24){
    //             for(k = 0;k<info_h.width;k++){
    //                 line_buf_set[k*4] = pic_buf_get[k*3];
    //                 line_buf_set[k*4+1] = pic_buf_get[k*3 +1];
    //                 line_buf_set[k*4+2] = pic_buf_get[k*3 +2];
    //                 line_buf_set[k*4+3] = 0;
    //                 //printf("%d %d %d\r\n",line_buf_set[k*3],line_buf_set[k*3+1],line_buf_set[k*3+2]);
    //             }
    //             memcpy(screen_base, line_buf_set, 3200);//刷入LCD显存
    //         }
            
    //     }
    // }

    /* 关闭文件、函数返回 */
    close(fd1);
    // free(screen_base);
    free(pic_buf_get);
    return 0;
}

/*切对应文件的一部分*/
int showcut_bmp_image(const char *path , int start_x, int start_y , int end_x , int end_y , int target_x , int target_y){
    bmp_file_header file_h;
    bmp_info_header info_h;
    char bit24plug = 0;    //字节对齐补充位
    unsigned char *pic_buf_get = NULL;    //原图缓冲区
    
    unsigned int pic_get_bytes;   //BMP图像一帧的字节的大小
    // unsigned int frame_cut_bytes;   //BMP图像实际裁剪后一帧的字节的大小
    // unsigned int frame_set_bytes;   //BMP图像实际裁剪后转为LCD显示深度一帧的字节的大小
    int cur_h, cur_w;
    int display_h , display_w;
    int fd0 , fd1 = -1;
    unsigned int color565,color888;
    int new_x = 0 , new_y = 0;
    int i,j,temp = 0;

    /* 打开framebuffer设备 */
    if (0 > (fd0 = open("/dev/fb0", O_RDWR))) {
        perror("open fb0 error");
        exit(EXIT_FAILURE);
    }

    /* 打开文件 */ 
    if (0 > (fd1 = open(path, O_RDONLY))) {
        perror("open error");
        return -1;
    }

    /* 读取BMP文件头 */
    if (sizeof(bmp_file_header) !=
        read(fd1, &file_h, sizeof(bmp_file_header))) {
        perror("read error");
        close(fd1);
        return -1;
    }

    if (0 != memcmp(file_h.type, "BM", 2)) {
        fprintf(stderr, "it's not a BMP file\n");
        close(fd1);
        return -1;
    }

    /* 读取位图信息头 */
    if (sizeof(bmp_info_header) !=
        read(fd1, &info_h, sizeof(bmp_info_header))) {
        perror("read error");
        close(fd1);
        return -1;
    }

    /* 打印信息 */
    // printf("\n<图片属性>\n文件大小: %d\n"
    //      "位图数据的偏移量: %d\n"
    //      "位图信息头大小: %d\n"
    //      "图像分辨率: %d*%d\n"
    //      "像素深度: %d\n"
    //      "图像大小: %d\n"
    //      , file_h.size, file_h.offset,
    //      info_h.size, info_h.width, info_h.height,
    //      info_h.bpp,info_h.image_size);
    
    /* 将文件读写位置移动到图像数据开始处 */
    if (-1 == lseek(fd1, file_h.offset, SEEK_SET)) {
        perror("lseek error");
        close(fd1);
        return -1;
    }
    
    /*计算数据补足的字节数*/
    bit24plug = info_h.width * info_h.bpp / 8 % 4;
    
    /* 申请一个buf、暂存bmp图像的一行数据 */
    pic_get_bytes = info_h.image_size;
    pic_buf_get = (unsigned char*)malloc(pic_get_bytes);
    if (NULL == pic_buf_get){
        fprintf(stderr, "malloc error\n");
        close(fd1);
        return -1;
    }
    // memset(screen_base, 0xFF, lcd_bytes_max);
    // memcpy(screen_base, screen_base, lcd_bytes_max);//刷入LCD显存

    /**** 计算图片显示的长宽 ****/
    if (0 < info_h.height){//倒向位图
        if (lcd_width > (end_x - start_x + target_x)){
            cur_w = end_x - start_x;
        }
        else{
            cur_w = lcd_width - target_x;
        }
        if (lcd_height > (end_y - start_y + target_y)){
            cur_h = end_y - start_y;
            //screen_base += width * (info_h.height - 1); //定位到....不知怎么描述 懂的人自然懂!
        }
        else {
            cur_h = lcd_height - target_y;
            //lseek(fd1, (info_h.height - lcd_height) * info_h.width, SEEK_CUR);
            //screen_base += width * (height - 1);    //定位到屏幕左下角位置
        }

        

        /**** 显示图片 ****/
        // printf("target_x:%d start_y:%d\r\n",target_x,start_y);
        // // printf("start_y:%d\r\n",start_y);
        //bit24plug = 0;
        read(fd1, pic_buf_get, pic_get_bytes); //读取出图像数据
        for (j = start_y , new_y = 0; j < start_y + cur_h; new_y ++ , j++){
            for (i = start_x , new_x = 0; i < start_x + cur_w; new_x ++ , i++){
                display_h = info_h.height - 1 - j;
                display_w = info_h.width;
                if(info_h.bpp == 32){
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 0] = pic_buf_get[(display_h * display_w + i) * 4 + 0];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 1] = pic_buf_get[(display_h * display_w + i) * 4 + 1];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 2] = pic_buf_get[(display_h * display_w + i) * 4 + 2];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 3] = pic_buf_get[(display_h * display_w + i) * 4 + 3];
                    
                }
                else if(info_h.bpp == 24){
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 0] = pic_buf_get[(display_h * display_w + i) * 3 + bit24plug * display_h + 0];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 1] = pic_buf_get[(display_h * display_w + i) * 3 + bit24plug * display_h + 1];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 2] = pic_buf_get[(display_h * display_w + i) * 3 + bit24plug * display_h + 2];
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 3] = 0;
                    temp ++ ;
                    
                    // printf("%d %d %x\r\n",i,j,screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 2]<<16+screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 1]<<8+screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 0]);
                }
                if(info_h.bpp == 16){
                    color565 = (pic_buf_get[(display_h * display_w + i)*2 + 1]<<8) + pic_buf_get[(display_h*display_w+i)*2];
                    color888 = RGB565toRGB888(color565);
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 0] = color888 & 0xff;
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 1] = (color888 >> 8) & 0xff;
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 2] = (color888 >> 16) & 0xff;
                    screen_base[((target_y + new_y) * lcd_width + new_x + target_x) * 4 + 3] = 0;
                    //printf("%d %d %d %d %d\r\n",color565,color888,screen_base[(j*min_w+i)*4],screen_base[(j*min_w+i)*4+1],screen_base[(j*min_w+i)*4+2]);
                    
                }
            }
            
        }
        // memcpy(screen_base, screen_base, lcd_bytes_max);//刷入LCD显存
    }
    // printf("bit24plug:%d , temp:%d\r\n",bit24plug,temp);
    /* 关闭文件、函数返回 */
    close(fd0);
    close(fd1);
    // free(screen_base);
    free(pic_buf_get);
    return 0;
}