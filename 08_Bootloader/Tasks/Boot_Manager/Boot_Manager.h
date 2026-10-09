#ifndef __BOOT_MANAGER_H
#define __BOOT_MANAGER_H


#include "main.h"


typedef  void (*pFunction)(void);



#define ApplicationAddress          0x8008000
#define NVIC_VectTab_FLASH          ((uint32_t)0x08000000)
#define BackApplicationAddress      0x8020000



void JumpToApp(void);



int8_t BackToApp(int32_t fl_size);



#endif


