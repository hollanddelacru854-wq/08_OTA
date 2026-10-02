/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOOT_MANAGER_H
#define __BOOT_MANAGER_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Exported types ------------------------------------------------------------*/
typedef  void (*pFunction)(void);
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define ApplicationAddress          0x8008000
#define NVIC_VectTab_FLASH          ((uint32_t)0x08000000)
#define BackApplicationAddress      0x8020000
/* Exported functions ------------------------------------------------------- */
void JumpToApp(void);
int8_t BackToApp(void);
#endif /* __BOOT_MANAGER_H */

