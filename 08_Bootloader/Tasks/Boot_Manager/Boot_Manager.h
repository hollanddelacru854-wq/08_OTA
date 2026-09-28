/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOOT_MANAGER_H
#define __BOOT_MANAGER_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef  void (*pFunction)(void);
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define ApplicationAddress          0x8008000
#define NVIC_VectTab_FLASH          ((uint32_t)0x08000000)
/* Exported functions ------------------------------------------------------- */
void JumpToApp(void);
#endif /* __BOOT_MANAGER_H */

