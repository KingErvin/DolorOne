KERNELDIR := /home/ez/linux/imx6ull/linux/linux-imx-ez_imx_4.1.15_2.1.0_ga
CURRENT_PATH := $(shell pwd)
CC := arm-linux-gnueabihf-gcc
CP := cp
TARGET_S := ez_DoloroneSinglethread
TARGET_M := ez_DoloroneMultithread
C_SOURCE_MAIN_S := ez_Dolorone_Singlethread.c 
C_SOURCE_MAIN_M := ez_Dolorone_Multithread.c 
C_SOURCE := $(wildcard ./source/*.c)
BMP_TARGET := /home/ez/linux/nfs/rootfs/lib/modules/4.1.15
BMP_SOURCE := $(wildcard ./ez_DoloroneSinglethread) $(wildcard ./ez_DoloroneMultithread) $(wildcard ./ez_project_dev.ko) $(wildcard ./picture)
LIB_PTHREAD = -lpthread
PATH_INCLUDE := -I
DEBUG_INCLUDE := -g

TARGET_ALL := $(TARGET_S) $(TARGET_M) $(BMP_TARGET)

#输出APP
$(TARGET_S):$(C_SOURCE) $(C_SOURCE_MAIN_S)
# $(CC) $^ -o $@ $(PATH_INCLUDE) $(DEBUG_INCLUDE)
	$(CC) $^ -o $@ $(DEBUG_INCLUDE)
	
$(TARGET_M):$(C_SOURCE) $(C_SOURCE_MAIN_M) 
	$(CC) $^ $(LIB_PTHREAD) -o $@ $(PATH_INCLUDE) $(DEBUG_INCLUDE)

#复制运行文件
$(BMP_TARGET):$(BMP_SOURCE)
	$(CP) -r $^ $@ -f

#生成KO驱动文件
obj-m := ez_project_dev.o

#运行所有流程
all : build $(TARGET_ALL) 

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean

	