#ifndef __OTA_H
#define __OTA_H


#include "main.h"
#include "Freertos.h"
#include "task.h"
#include "cmsis_os.h"


#define BUFFER_SIZE                     1030



extern void ota_task_runnable(void *argument);



#endif
