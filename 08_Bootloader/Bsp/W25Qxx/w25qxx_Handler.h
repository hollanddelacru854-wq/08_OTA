#ifndef __W25Q_HANDLER_H
#define __W25Q_HANDLER_H


#include "main.h"
#include "stm32f4xx.h"


/*
	最小读写单元Pag = 256 byte
	一个扇区 = 16个Pag = 4096 byte
	一个块 = 16个扇区 = 64KB
*/
typedef struct 
{
    u8  databuf[4096];           //按找4096个数据进行读写设置（缓冲区）
    u16 write_databuf_index;
    u32 write_index;
    u8  write_sector_index;      //4096
    u32 read_index;
    u8  read_sector_index;
}st_W25Q_Handler;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define EXTERN_Flash

/* Exported functions ------------------------------------------------------- */
void W25Q64_Init(void);
u8 W25Q64_EraseChip(void);
u8 W25Q64_WriteData(u8 *data, u32 length);
u8 W25Q64_WriteData_End(void);
u8 W25Q64_ReadData(u8 *data, u16 *length);
#endif /* __FLASH_H */
