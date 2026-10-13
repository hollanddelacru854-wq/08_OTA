//独立的OTA下载任务

#include "OTA.h"
#include "ymodem.h"


osThreadId_t OTA_taskHandle;
const osThreadAttr_t OTA_task_attributes = {
  .name = "OTA_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};


uint8_t g_au8_YmodemRec[1030];




void ota_task_runnable(void *argument)
{
  
	
  for(;;)
  {
	  
    Ymodem_Receive(g_au8_YmodemRec);
	  
  }
  

  
}

