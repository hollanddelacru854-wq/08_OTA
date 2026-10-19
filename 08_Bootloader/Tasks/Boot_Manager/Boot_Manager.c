
//1.从bootloader程序跳转到APP程序
//2.将接收到的新固件进行解密


#include "Boot_Manager.h"
#include "main.h"
#include "Flash.h"
#include "AES.h"
#include "w25qxx_Handler.h"
#include "w25qxx.h"
#include "elog.h"
#include "AT24Cxx_Driver.h"
#include "Gpio.h"
#include "ymodem.h"



//接收数据的缓冲区
uint8_t au8_test[1024]; 

uint32_t JumpAddress;
int32_t app_size = 0;


//完成从bootloader到APP的安全跳转
void JumpToApp(void)
{
    /* 检查栈顶地址是否合法 */
    uint32_t JumpAddress;
    pFunction Jump_To_Application;

    /* 检查栈顶地址是否合法 */
    if(((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
    {
        /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
        __disable_irq();
        NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
        RCC_DeInit();
        /* 用户代码区第二个 字 为程序开始地址(复位地址) */
        JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);

        /* Initialize user application's Stack Pointer */
        /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
        __set_MSP(*(__IO uint32_t *) ApplicationAddress);

        /* 类型转换 */
        Jump_To_Application = (pFunction) JumpAddress;

        /* 跳转到 APP */
        Jump_To_Application();
    }
}




////将MCU接收的固件写入备份区
//int8_t BackToApp(void)
//{
//    int32_t j = 0;
//    uint32_t FlashDestination = ApplicationAddress;
//    uint32_t BackflashSource;
//    if ((app_size > (0x18000 - 1)) ||\
//        (app_size < 0))
//    {
//        return -1;
//    }
//    BackflashSource = BackApplicationAddress;
//    for (j = 0;j < app_size; j+= 4)
//    {
//      Flash_Write(FlashDestination,*(uint32_t*)BackflashSource);
//      if (*(uint32_t*)FlashDestination != *(uint32_t*)BackflashSource)
//      {
//        return -1;
//      }
//      FlashDestination += 4;
//      BackflashSource += 4;
//    }
//    return 0;
//}





//使用AES加密算法库实现新固件解密（备份区中的新的加密固件）
//先从外部Flash中读取数据包头并解析，然后将待写入区擦除，
//最后从外部Flash中读取数据并放入缓冲区，解析并写入内部Flash
unsigned char IV[16]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};  
unsigned char Key[32]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,\
                       0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};
u8  Mem_Read_buffer[4096];
int8_t BackToApp(int32_t fl_size)
{
    u8 Temp[16];  //原密文数据缓存
    u8 wirteTime=0;  //一个解析包写入次数
    u16 readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节）
    u32 AppSize=0;  //升级包的大小
    //u32 FlashDestination=ApplicationAddress;
    u16 Read_Memory_Size=0;
    u32 Read_Memory_index=0;
    uint8_t *pu8_IV_IN_OUT = IV;
    uint8_t *pu8_key256bit = Key;
    uint32_t RamSource = 0;
    uint32_t AppRunFlashDestination = ApplicationAddress;
    if(fl_size <= 0)
    {
      return -1;
    }
    if ((app_size > (0x18010 - 1)) || (app_size < 0))
    {
      return -1;
    }
    //先读一帧，用来解析头文件格式（从外部Flash中读取获得新固件）
    W25Q64_ReadData(BLOCK_2, Mem_Read_buffer, &Read_Memory_Size);
    if(Read_Memory_Size  >= 16)
    {
      memcpy(Temp,Mem_Read_buffer,16);
      Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析得到自定义内容+文件大小
      AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+Temp[12];
      log_d("AppSize=%d",AppSize);
      
      //计算升级包读取次数
      readDataCount=AppSize/16;
      if(AppSize%16!=0)
      {
        readDataCount+=1;
      }
      Read_Memory_index += 16;
    }

    //数据帧
    //将待写入区的内容擦除
    uint8_t flash_erase_state = Flash_erase(AppRunFlashDestination,AppSize);
    if(flash_erase_state == 0)
    {
      for(readTime=0;readTime<readDataCount;readTime++)
      {
        //判断下当前buffer下的数据是否读取完毕
        if(Read_Memory_index == Read_Memory_Size)
        {
          if(2 == W25Q64_ReadData(BLOCK_2, Mem_Read_buffer,&Read_Memory_Size))
          {
            log_d("Write_Flash_After_AES_Decode read extern buffer error");
            return -1;
          }
          Read_Memory_index = 0;
        }
        //拷贝16个数据
        memcpy(Temp,Mem_Read_buffer + Read_Memory_index,16);
        Read_Memory_index += 16;
        //解析16个数据
        Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析
        
        RamSource = (uint32_t)Temp;
        for (wirteTime = 0;wirteTime<4;wirteTime++)
        {
          Flash_Write(AppRunFlashDestination, *(uint32_t*)RamSource);
          AppRunFlashDestination += 4;
          RamSource += 4;
        }
      }
      log_d("Write_Flash_After_AES_Decode end");
      return 0;
    }
    else
    {
      log_d("Write_Flash_After_AES_Decode Error");
      return -1;
    }
}




//OTA状态机
void OTA_StateManager(void)
{
  uint8_t t_u8_otastate = NO_APP_UPDATE;
  uint32_t t_u32_appsize = 0;
  int32_t fil_size = 0;
  /*读eeprom的OTA状态*/
  ee_ReadBytes(&t_u8_otastate,0x00,1);

  switch(t_u8_otastate)
  {
    case NO_APP_UPDATE:
      if(Key_Scan())
      {
        //按下
        /*1.下载到外部flash的A区*/
        fil_size = Ymodem_Receive(au8_test);
        /*2.解密数据拷贝到外部flashB区中*/
        //if(0 == BackToApp(fil_size))
        // {
        //   JumpToApp();
        // }
        // else
        // {
        //   //
        // }
      }
      else
      {
        JumpToApp();
      }
    break;

    case APP_DOWNLOADING:
      log_a("App dowload failed");
      JumpToApp();

      //跳转失败，下载新App

    break;

    case APP_DOWNLOAD_COMPLETE:
      //读取当前需要更新的App的大小
      ee_ReadBytes((uint8_t *)&t_u32_appsize,0x01,4);

      //解密搬运数据
    break;

    default:
    /*No action*/
    break;
  }
}












