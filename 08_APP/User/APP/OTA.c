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

osThreadId_t OTA_taskHandle;
const osThreadAttr_t OTA_task_attributes = {
	
  .name = "OTA_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
	
};


static int8_t Key_Scan();
void SoftReset(void);


//固件缓冲区
uint8_t g_au8_YmodemRec[1030];


//状态
static OTA_State s_OTA_State = WaitReqDownload;


//命令缓冲区
static uint8_t s_au8_OtaCmd[4];


//队列
extern QueueHandle_t Q_YmodemReclength;




void ota_task_runnable(void *argument)
{
	
  uint8_t t_u8_rec_length = 0;
  uint8_t t_au8_AckCmd[] = {0x44,0x55,0x66};
	
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
			//打开串口接收，并切换为状态3（等待固件接收完成）
			if(0 < Ymodem_Receive(g_au8_YmodemRec))
			{
			  s_OTA_State = WaitReqUpdate;
			  
			  HAL_UART_Transmit(&huart1,t_au8_AckCmd,3,1000);

			  
			}
			else
			{
			  s_OTA_State = WaitReqDownload;
			  
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



static int8_t Key_Scan()
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




void SoftReset(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}







