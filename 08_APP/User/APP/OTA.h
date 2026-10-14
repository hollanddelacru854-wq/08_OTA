#ifndef __OTA_H
#define __OTA_H


#include "main.h"
#include "Freertos.h"
#include "task.h"
#include "cmsis_os.h"


#define BUFFER_SIZE                     1030

//OTA的四种状态

/*
1.等待下载
2.OTA下载中
3.等待用户允许更新
4.OTA下载完成
*/

typedef enum
{
    WaitReqDownload = 0,		
    OtaDownload,
    WaitReqUpdate,
    OtaEnd
}OTA_State;



extern void ota_task_runnable(void *argument);



#endif
