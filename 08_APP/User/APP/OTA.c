//OTA状态管理


/*

四种状态：

1.等待下载
2.OTA下载中
3.等待用户允许更新
4.OTA下载完成


*/


#include "OTA.h"
#include "ymodem.h"
#include "usart.h"
#include "gpio.h"
#include "W25Q_Handler.h"
#include "AT24Cxx_Driver.h"



osThreadId_t OTA_taskHandle;
const osThreadAttr_t OTA_task_attributes = {
	
  .name = "OTA_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
	
};


osThreadId_t DownloadAppData_taskHandle;
const osThreadAttr_t DownloadAppData_task_attributes = {
  .name = "DownloadAppData_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};


QueueHandle_t Queue_AppDataBuffer;


SemaphoreHandle_t Semaphore_ExtFlashState;


//函数声明
static int8_t Key_Scan(void);
void SoftReset(void);
void DownloadAppData_task_runnable(void *argument);


//固件缓冲区
//uint8_t g_au8_YmodemRec[1030];

uint8_t g_au8_YmodemRec_A[1030];

uint8_t g_au8_YmodemRec_B[1030];


//状态
static OTA_State s_OTA_State = WaitReqDownload;


//命令缓冲区
static uint8_t s_au8_OtaCmd[4];


//传递固件长度的队列
extern QueueHandle_t Q_YmodemReclength;


extern int32_t packet_length;

void ota_task_runnable(void *argument)
{
	
  uint8_t t_u8_rec_length = 0;
  uint8_t t_au8_AckCmd[] = {0x44,0x55,0x66};
  int32_t t_int32_app_data_length = 0;

  uint8_t t_u8_otastate = 0x00;
  //清除AT24C02中的OTA状态位
  ee_WriteBytes(&t_u8_otastate,0x00,1);
  
  
  for(;;)
  {
	  
	  
	  
//    Ymodem_Receive(g_au8_YmodemRec);
	  
	  switch(s_OTA_State)
	  {
		  case WaitReqDownload:
			  
			//阻塞接收串口命令
			HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_au8_OtaCmd, 4);
			
		    //等待命令接收完成
			xQueueReceive(Q_YmodemReclength,&t_u8_rec_length,portMAX_DELAY);
		  
		  
			//校验命令长度
			if(3 == t_u8_rec_length)
			{
			  //校验命令
			  if((0x11 == s_au8_OtaCmd[0]) && (0x22 == s_au8_OtaCmd[1]) && (0x33 == s_au8_OtaCmd[2]))
			  {
				  
				//切换到状态2
				s_OTA_State = OtaDownload;
				
				//创建线程以及相关队列（有数据才创建写入线程，节省资源）
				DownloadAppData_taskHandle = osThreadNew(DownloadAppData_task_runnable, NULL, &DownloadAppData_task_attributes);
				Queue_AppDataBuffer = xQueueCreate(2,sizeof(uint8_t *));
				Semaphore_ExtFlashState = xSemaphoreCreateMutex();

				//重写AT24C02中的OTA状态位（即表示APP更新中）
				t_u8_otastate = 0x11;
				ee_WriteBytes(&t_u8_otastate,0x00,1);
				 
			  }
			  else
			  {
				  
				//清空命令缓冲区
				s_au8_OtaCmd[0] = 0;
				s_au8_OtaCmd[1] = 0;
				s_au8_OtaCmd[2] = 0;
				s_au8_OtaCmd[3] = 0;
			  }
			}
			else
			{
				
			  //清空命令缓冲区
			  s_au8_OtaCmd[0] = 0;
			  s_au8_OtaCmd[1] = 0;
			  s_au8_OtaCmd[2] = 0;
			  s_au8_OtaCmd[3] = 0;
				
			}
		  break;
		  
		  case OtaDownload:
			//开始接收新固件
			t_int32_app_data_length = Ymodem_Receive(g_au8_YmodemRec_A,g_au8_YmodemRec_B);
			
			
			if(0 < t_int32_app_data_length)
			{
				
			  //打开串口接收，并切换为状态3（等待固件接收完成）
			  s_OTA_State = WaitReqUpdate;
			  
			  HAL_UART_Transmit(&huart1,t_au8_AckCmd,3,1000);

			  
			  //获取并释放信号量
			  xSemaphoreTake(Semaphore_ExtFlashState,portMAX_DELAY);
			  xSemaphoreGive(Semaphore_ExtFlashState);

			  //执行最后一次写入（补齐最后一块）
			  W25Q64_WriteData_End();
			  
			  //清除一切生成的资源
			  vTaskDelete(DownloadAppData_taskHandle);
			  vQueueDelete(Queue_AppDataBuffer);
			  vSemaphoreDelete(Semaphore_ExtFlashState);

			  //重写AT24C02中的OTA状态位（即表示APP请求更新）
			  t_u8_otastate = 0x22;
			  ee_WriteBytes(&t_u8_otastate,0x00,1);

			  //Test 
			  uint8_t t_u8_readstate = 0;
			  ee_ReadBytes(&t_u8_readstate,0x00,1);
			  HAL_UART_Transmit(&huart1,&t_u8_readstate,1,1000);


			  //写入数据长度
			  ee_WriteBytes((uint8_t *)&t_int32_app_data_length,0x01,sizeof(t_int32_app_data_length));


			  //Test 
			  uint8_t t_au8_readlength[4] = {0};
			  ee_ReadBytes(t_au8_readlength,0x01,4);
			  HAL_UART_Transmit(&huart1,t_au8_readlength,4,1000);
				
				
			}
			else
			{
			  s_OTA_State = WaitReqDownload;
				
			  /*清除一切生成的资源*/
			  vTaskDelete(DownloadAppData_taskHandle);
			  vQueueDelete(Queue_AppDataBuffer);
			  vSemaphoreDelete(Semaphore_ExtFlashState);
			}
		  break;
		  
		  case WaitReqUpdate:
			  
			//重新开启串口+DMA接收完成命令
			HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_au8_OtaCmd, 4);
		  
			//等待命令接收完成
			xQueueReceive(Q_YmodemReclength,&t_u8_rec_length,portMAX_DELAY);
			
		    //校验完成命令长度
			if(3 == t_u8_rec_length)
			{
			  //校验命令内容
			  if((0x77 == s_au8_OtaCmd[0]) && (0x88 == s_au8_OtaCmd[1]) && (0x99 == s_au8_OtaCmd[2]))
			  {
				//切换到状态4
				s_OTA_State = OtaEnd;
			  }
			  else
			  {
				s_au8_OtaCmd[0] = 0;
				s_au8_OtaCmd[1] = 0;
				s_au8_OtaCmd[2] = 0;
				s_au8_OtaCmd[3] = 0;
			  }
			}
			else
			{
			  s_au8_OtaCmd[0] = 0;
			  s_au8_OtaCmd[1] = 0;
			  s_au8_OtaCmd[2] = 0;
			  s_au8_OtaCmd[3] = 0;
			}
		  break;

		  
		  case OtaEnd:
			//等待用户按下按键
			if(1 == Key_Scan())
			{
			  //执行软复位
			  SoftReset();
			}
			else
			{
			  //无限期阻塞
			  s_OTA_State = WaitReqDownload;
			}
		  break;
		  
		  
		  default:
		  
		  break;
		  
	  }

	  
  }
  

  
}



static int8_t Key_Scan(void)
{
  uint16_t t_u16_cnt = 0;

  for(;t_u16_cnt < 400; t_u16_cnt++)
  {
    if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(Key_GPIO_Port,Key_Pin))
    {
      osDelay(20);
      if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(Key_GPIO_Port,Key_Pin))
      {
        //按键按下
        return 1;
      }
    }
    osDelay(50);
  }
  //按键超时
  return -1;
}



//软件复位
void SoftReset(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}


void DownloadAppData_task_runnable(void *argument)
{
  uint8_t * pu8_data = NULL;
  int32_t * pu32_size = NULL;
	
  //第一帧数据一定是数据的长度
  xQueueReceive(Queue_AppDataBuffer,&pu32_size,portMAX_DELAY);
	
  
  xSemaphoreGive(Semaphore_ExtFlashState);
  while(1)
  {
	  
    xQueueReceive(Queue_AppDataBuffer,&pu8_data,portMAX_DELAY);
	  
	//先获取信号量后再执行外部Flash的逻辑（防止写入时有新的数据）
    xSemaphoreTake(Semaphore_ExtFlashState,0);
    if(pu8_data == NULL)
    {
      continue;
    }
	
    //执行写入W25q的逻辑
    W25Q64_WriteData(pu8_data,(uint32_t)packet_length);
    xSemaphoreGive(Semaphore_ExtFlashState);
	
  }
}








