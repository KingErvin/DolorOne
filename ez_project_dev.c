#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/string.h>
#include <linux/interrupt.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名		: ezdev.c
作者	  	: 左忠凯
版本	   	: V1.0
描述	   	: LED驱动文件。
其他	   	: 无
论坛 	   	: www.openedv.com
日志	   	: 初版V1.0 2019/6/27 左忠凯创建
***************************************************************/
#define ezdev_CNT			1		  	/* 设备号个数 */
#define ezdev_NAME			"ezdev"	/* 名字 */
#define LEDOFF 					0			/* 关灯 */
#define LEDON 					1			/* 开灯 */
 
/*传输的数据序号*/
#define TRANSLATE_MAX 4
#define TRANSLATE_BTN 0
#define TRANSLATE_HMI 1
#define TRANSLATE_SHOCK 2
#define TRANSLATE_PRESSURE 3

/*定时时间*/
#define TIMER_PEROID_HMI 30
#define TIMER_PEROID_SHOCK 1
#define TIMER_PEROID_PRESSURE 1000

/* 映射后的寄存器虚拟地址指针 */
// static void __iomem *IMX6U_CCM_CCGR1;
// static void __iomem *SW_MUX_GPIO1_IO03;
// static void __iomem *SW_PAD_GPIO1_IO03;
// static void __iomem *GPIO1_DR;
// static void __iomem *GPIO1_GDIR;

// static void __iomem *SW_MUX_GPIO1_IO18;
// static void __iomem *SW_PAD_GPIO1_IO18;

/* ezdev设备结构体 */
struct ez_timer{

	int value; /*键值*/

	struct timer_list timer;
	int timeperiod; //延迟时间
	struct tasklet_struct tasklet; 
};


struct ez_btn{
	int gpio;
	int state;

	int value; /*键值*/
	int release_flag; /*释放标志*/

	int irq_num;
	char name[20]; /*名字*/

	irqreturn_t (*handler)(int,void*); /*中断处理函数*/

	struct timer_list timer;
	
	int timeperiod; //延迟时间

	struct tasklet_struct tasklet; 
};

struct ez_led{
	int gpio;
	int state;
};

struct ez_beep{
	int gpio;
	int state;
};

struct ez_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node *nd;
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct ez_led led;
	struct ez_btn btn;
	struct ez_beep beep;
	struct ez_timer mt;
	struct ez_timer shock;
	struct ez_timer pressure;
};

struct ez_dev ezdev;	/* 设备 */
int sta = 0;
/*
 * @description		: LED常打开/关闭
 * @param - sta 	: LEDON(0) 打开LED，LEDOFF(1) 关闭LED
 * @return 			: 无
 */
void led_push(u8 sta){
	gpio_set_value(ezdev.led.gpio,sta);
}

/*
 * @description		: BEEP短暂打开/常关闭
 * @param - sta 	: LEDON(0) 打开LED，LEDOFF(1) 关闭LED
 * @return 			: 无
 */
void beep_push(u8 sta){
	gpio_set_value(ezdev.beep.gpio,sta);
}
/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int ez_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ezdev; /* 设置私有数据 */
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */

static ssize_t btn_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int val = 0;
	int databuff[5];

	databuff[TRANSLATE_BTN] = ezdev.btn.release_flag;
	databuff[TRANSLATE_HMI] = ezdev.mt.value;
	databuff[TRANSLATE_SHOCK] = ezdev.shock.value;
	databuff[TRANSLATE_PRESSURE] = ezdev.pressure.value;
	val = copy_to_user(buf, databuff, sizeof(databuff));
	return val;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t ez_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned int databuff[10];

	retvalue = copy_from_user(databuff, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}
	ezdev.btn.release_flag = databuff[TRANSLATE_BTN];
	ezdev.mt.value = databuff[TRANSLATE_HMI];
	ezdev.shock.value = databuff[TRANSLATE_SHOCK];
	ezdev.pressure.value = databuff[TRANSLATE_PRESSURE];
	// sta = 0;
	// led_push(~ezdev.shock.value);
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int ez_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations ezdev_fops = {
	.owner = THIS_MODULE,
	.open = ez_open,
	.read = btn_read,
	.write = ez_write,
	.release = 	ez_release,
};

/*定时中断函数*/
static irqreturn_t btn0_handler(int irq,void *dev_id){

	struct ez_btn *dev = (struct ez_btn *)dev_id;

	tasklet_schedule(&dev->tasklet);
	
	// printk("离开按键中断\n");
	return IRQ_HANDLED;
}

static void btn_tasklet(unsigned long data){

	struct ez_btn *dev = (struct ez_btn*)data;

	// dev->timeperiod = 50;
	printk("进入按键中断，消抖定时%dms\n",dev->timeperiod);
	dev->timer.data = data;
	mod_timer(&dev->timer , jiffies + msecs_to_jiffies(dev->timeperiod));

}


/*定时处理函数*/
void timer_btn_func(unsigned long arg){
	struct ez_btn *dev = (struct ez_btn*)arg;
	// static int sta = 1;

	// printk("进入定时器\n");
	dev->value = gpio_get_value(dev->gpio);
	printk("当前BTN状态:%d\r\n",dev->value);
	if(dev->value)
		dev->release_flag = 1;
	// led_push(dev->value);
	// beep_push(dev->value);
	// printk("离开定时器\n");
}

void timer_mt_func(unsigned long arg){
	struct ez_timer *dev = (struct ez_timer*)arg;
	// static int sta = 1;

	// printk("进入定时器\n");
	dev->value = 1;
	// printk("触控检测，定时%dms value:%d\n",dev->timeperiod,dev->value);
	
	mod_timer(&dev->timer , jiffies + msecs_to_jiffies(dev->timeperiod));
	//beep_push(readbit);
	// printk("离开定时器\n");
}

void timer_shock_func(unsigned long arg){
	struct ez_timer *dev = (struct ez_timer*)arg;
	
	// sta ++;
	// printk("进入定时器\n");
	dev->value = 1;
	// led_push(sta);
	// printk("LED：%d\n",sta);
	// printk("冲击检测，定时%dms value:%d time:%ld\n",dev->timeperiod,dev->value,msecs_to_jiffies(dev->timeperiod));
	
	mod_timer(&dev->timer , jiffies + msecs_to_jiffies(dev->timeperiod));
	//beep_push(readbit);
	// printk("离开定时器\n");
}

void timer_pressure_func(unsigned long arg){
	struct ez_timer *dev = (struct ez_timer*)arg;
	
	// sta ++;
	// printk("进入定时器\n");
	dev->value = 1;
	// led_push(sta);
	// printk("LED：%d\n",sta);
	// printk("冲击检测，定时%dms value:%d time:%ld\n",dev->timeperiod,dev->value,msecs_to_jiffies(dev->timeperiod));
	
	mod_timer(&dev->timer , jiffies + msecs_to_jiffies(dev->timeperiod));
	//beep_push(readbit);
	// printk("离开定时器\n");
}

/*定时器初始化*/
int timer_btn_init(struct ez_btn *dev){
	init_timer(&dev->timer);
	dev->timer.function = timer_btn_func;
	dev->timeperiod = 500;
	// ezdev.btn.timeperiod = 100;
	// ezdev.btn.timer_btn.expires = jiffies + msecs_to_jiffies(ezdev.btn.timeperiod);
	// ezdev.btn.timer_btn.data = (unsigned long)&ezdev.btn;
	// add_timer(&ezdev.btn.timer_btn);
	return 0;
}

int timer_init(struct ez_timer *dev , void (*func)(unsigned long) , int timeperiod){
	init_timer(&dev->timer);
	dev->timer.function = func;
	dev->timeperiod = timeperiod; //ms
	dev->timer.expires = jiffies + msecs_to_jiffies(dev->timeperiod);
	dev->timer.data = (unsigned long)dev;
	mod_timer(&dev->timer , jiffies + msecs_to_jiffies(dev->timeperiod));
	// printk("冲击检测，定时%dms value:%d time:%ld\n",dev->timeperiod,dev->value,msecs_to_jiffies(dev->timeperiod));
	return 0;
}

int btninit(struct ez_btn *dev){
	int ret;
	const char *str;
	/*按键初始化*/
	ezdev.nd = of_find_node_by_path("/ezbtn");
	ret = of_property_read_string(ezdev.nd,"status",&str);
	if(ret>=0){
		printk("btn status:%s\r\n",str);
	}
	ret = of_property_read_string(ezdev.nd,"compatible",&str);
	if(ret>=0){
		printk("btn compatible:%s\r\n",str);
	}
	dev->gpio = of_get_named_gpio(ezdev.nd,"btn-gpios",0);
	printk("ezdev.btn.gpio:%d\r\n",dev->gpio);

	memset(dev->name,0,sizeof(dev->name));
	sprintf(dev->name,"BTN1");
	ret = gpio_request(dev->gpio,dev->name);
	if(ret<0){
		printk("btn request error\r\n");
	}

	/*中断初始化*/
	//dev->irq_num = gpio_to_irq(dev->gpio);
	dev->irq_num = irq_of_parse_and_map(ezdev.nd,0);

	ret = request_irq(dev->irq_num,
				btn0_handler,
				IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
				dev->name,
				&ezdev.btn);
	if(ret<0){
		printk("btn IRQ:%d request error\r\n",0);
	}
	else{
		printk("btn IRQ name:%s;irq_num:%d\r\n",dev->name,dev->irq_num);
	}

	tasklet_init(&dev->tasklet,btn_tasklet,(unsigned long)dev);
	// ret = gpio_direction_input(dev->gpio);
	// if(ret>=0){
	// 	printk("btn dir:%d\r\n",ret);
	// }
	
	printk("\n");
	return 0;
}

int beepinit(struct ez_beep *dev){
	int ret;
	const char *str;

	ezdev.nd = of_find_node_by_path("/ezbeep");
	ret = of_property_read_string(ezdev.nd,"status",&str);
	if(ret>=0){
		printk("beep status:%s\r\n",str);
	}
	ret = of_property_read_string(ezdev.nd,"compatible",&str);
	if(ret>=0){
		printk("beep compatible:%s\r\n",str);
	}
	dev->gpio = of_get_named_gpio(ezdev.nd,"beep-gpios",0);
	printk("ezdev.beep.gpio:%d\r\n",dev->gpio);
	
	ret = gpio_request(dev->gpio,NULL);
	if(ret<0){
		printk("beep request error\r\n");
	}

	ret = gpio_direction_output(dev->gpio,1);
	if(ret>=0){
		printk("beep dir:%d\r\n",ret);
	}
	return 0;
}

int ledinit(struct ez_led *dev){
	int ret;
	const char *str;

	ezdev.nd = of_find_node_by_path("/ezled");
	ret = of_property_read_string(ezdev.nd,"status",&str);
	if(ret>=0){
		printk("led status:%s\r\n",str);
	}
	ret = of_property_read_string(ezdev.nd,"compatible",&str);
	if(ret>=0){
		printk("led compatible:%s\r\n",str);
	}
	dev->gpio = of_get_named_gpio(ezdev.nd,"led-gpios",0);
	printk("ezdev.led.gpio:%d\r\n",dev->gpio);
	
	ret = gpio_request(dev->gpio,NULL);
	if(ret<0){
		printk("led request error\r\n");
	}

	ret = gpio_direction_output(dev->gpio,1);
	if(ret>=0){
		printk("led dir:%d\r\n",ret);
	}
	
	printk("\n");
	return 0;
}


/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init ez_init(void)
{
	int ret = 0;

	btninit(&ezdev.btn);
	ledinit(&ezdev.led);
	beepinit(&ezdev.beep);
	timer_btn_init(&ezdev.btn);
	timer_init(&ezdev.mt , timer_mt_func , TIMER_PEROID_HMI);
	timer_init(&ezdev.shock , timer_shock_func, TIMER_PEROID_SHOCK);
	timer_init(&ezdev.pressure , timer_pressure_func , TIMER_PEROID_PRESSURE);

	led_push(1);
	beep_push(1);

	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (ezdev.major) {		/*  定义了设备号 */
		ezdev.devid = MKDEV(ezdev.major, 0);
		ret = register_chrdev_region(ezdev.devid, ezdev_CNT, ezdev_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&ezdev.devid, 0, ezdev_CNT, ezdev_NAME);	/* 申请设备号 */
		ezdev.major = MAJOR(ezdev.devid);	/* 获取分配号的主设备号 */
		ezdev.minor = MINOR(ezdev.devid);	/* 获取分配号的次设备号 */
	}
	if(ret < 0 ){
		goto fail_devid;
	}
	else{
		printk("ez_project major=%d,minor=%d\r\n",ezdev.major, ezdev.minor);	
	}
	
	/* 2、初始化cdev */
	ezdev.cdev.owner = THIS_MODULE;
	cdev_init(&ezdev.cdev, &ezdev_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&ezdev.cdev, ezdev.devid, ezdev_CNT);

	/* 4、创建类 */
	ezdev.class = class_create(THIS_MODULE, ezdev_NAME);
	if (IS_ERR(ezdev.class)) {
		return PTR_ERR(ezdev.class);
	}

	/* 5、创建设备 */
	ezdev.device = device_create(ezdev.class, NULL, ezdev.devid, NULL, ezdev_NAME);
	if (IS_ERR(ezdev.device)) {
		return PTR_ERR(ezdev.device);
	}
	
	return 0;

fail_devid:
	return ret;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit ez_exit(void)
{
	/* 取消映射 */
	// iounmap(IMX6U_CCM_CCGR1);
	// iounmap(SW_MUX_GPIO1_IO03);
	// iounmap(SW_PAD_GPIO1_IO03);
	// iounmap(GPIO1_DR);
	// iounmap(GPIO1_GDIR);
	// iounmap(SW_MUX_GPIO1_IO18);
	// iounmap(SW_PAD_GPIO1_IO18);
	del_timer_sync(&ezdev.btn.timer);
	del_timer_sync(&ezdev.mt.timer);
	del_timer_sync(&ezdev.shock.timer);
	del_timer_sync(&ezdev.pressure.timer);

	free_irq(ezdev.btn.irq_num,&ezdev.btn);

	gpio_free(ezdev.beep.gpio);
	gpio_free(ezdev.led.gpio);
	gpio_free(ezdev.btn.gpio);

	/* 注销字符设备驱动 */
	cdev_del(&ezdev.cdev);/*  删除cdev */
	unregister_chrdev_region(ezdev.devid, ezdev_CNT); /* 注销设备号 */

	device_destroy(ezdev.class, ezdev.devid);
	class_destroy(ezdev.class);
	
	
}

module_init(ez_init);
module_exit(ez_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ervin Zhu");
