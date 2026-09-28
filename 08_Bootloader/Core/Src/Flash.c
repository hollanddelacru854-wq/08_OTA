/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file Flash.c
 * 
 * @par dependencies 
 * - Flash.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Functions related to reading and writing in the chip's flash area.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "Flash.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* extern variables ---------------------------------------------------------*/

void Flash_Unlock(void) {
    FLASH_Unlock();
    while (FLASH_GetStatus() == FLASH_BUSY);
}

void Flash_Lock(void) {
    FLASH_Lock();
}

//擦除APP区域的数据
// f4是按扇区操作，计划将app放在扇区6  FLASH_Sector_6，备份app放在扇区7
FLASH_Status EreaseAppSector(uint32_t FLASH_Sector)
{
	Flash_Unlock();
	FLASH_Status FLASHStatus = FLASH_EraseSector(FLASH_Sector, VoltageRange_3);
	Flash_Lock();
	return FLASHStatus;
}

void Flash_Write(uint32_t address, uint32_t data) {
    // 解锁Flash
    Flash_Unlock();
    
    // 清除所有标志位
	  // FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    
    // 编程一个字（32位）
    FLASH_Status status = FLASH_ProgramWord(address, data);
    
    // 检查编程是否成功
    if (status == FLASH_COMPLETE) {
        // 数据写入成功
			  // log_d("Flash_Write success"); 
    } else {
        // 数据写入失败
        // 在这里添加错误处理代码
			  //log_e("Flash_Write fail"); 
    }
    
    // 锁定Flash
    Flash_Lock();
}



