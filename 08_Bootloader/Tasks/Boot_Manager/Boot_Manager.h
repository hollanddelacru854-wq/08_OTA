#ifndef __BOOT_MANAGER_H
#define __BOOT_MANAGER_H


#include "main.h"


typedef  void (*pFunction)(void);



#define ApplicationAddress          0x8008000			//APP运行区
#define NVIC_VectTab_FLASH          ((uint32_t)0x08000000)
#define BackApplicationAddress      0x8020000			//备份区（内部Flash）



//AT24C02中的状态参数
#define NO_APP_UPDATE               0x00
#define APP_DOWNLOADING             0x11
#define APP_DOWNLOAD_COMPLETE       0x22



void JumpToApp(void);



int8_t BackToApp(int32_t fl_size);



#endif


