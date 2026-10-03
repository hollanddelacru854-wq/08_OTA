#include "Boot_Manager.h"
#include "main.h"
#include "Flash.h"
#include "AES.h"

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



//使用AES加密算法库实现新固件解密（备份区中的新的加密固件），并将新固件写入运行区
unsigned char IV[16]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};  
unsigned char Key[32]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,\
                       0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};
int8_t BackToApp(int32_t fl_size)
{
  uint32_t AppRunFlashDestination = ApplicationAddress;
  uint8_t *pu8_IV_IN_OUT = IV;
  uint8_t *pu8_key256bit = Key;
  uint8_t *pu8_temp = (uint8_t *)BackApplicationAddress;  //原始数据
  uint8_t Temp[16];  //原密文数据缓存
  uint8_t *pTemp = Temp;
  uint16_t readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节
  u32 AppSize=0;  //升级包的大小

  if(fl_size <= 0)
  {
    return -1;
  }

  //校验
  if ((app_size > (0x18010 - 1)) ||\
      (app_size < 0))
  {
    return -1;
  }
  
  memcpy(pTemp,pu8_temp,16);
  pu8_temp += 16;
  Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,pTemp,pu8_key256bit);//解析得到自定义内容+文件大小
  AppSize=(pTemp[15]<<24)+(pTemp[14]<<16)+(pTemp[13]<<8)+pTemp[12];

  /*计算需要解密多少次*/
  readDataCount=AppSize/16;
  if(AppSize%16!=0)
  {
    readDataCount+=1;
  }

  //擦除运行区数据
  if(1 == Flash_erase(ApplicationAddress,AppSize))
  {
    return -1;
  }
  
  //读数据的总次数
  for(readTime=0;readTime<readDataCount;readTime++)
  {
    //加密原文读取16个字节到临时区中
    pTemp = Temp;
    memcpy(pTemp,pu8_temp,16);
    pu8_temp += 16;
    Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,pTemp,pu8_key256bit);//解密数据
    //解密后的数据存入App运行区中（每次搬运完进行校验）
    for (uint8_t j = 0;j < 16; j+= 4)
    {
      Flash_Write(AppRunFlashDestination,*(uint32_t*)pTemp);
      if (*(uint32_t*)AppRunFlashDestination != *(uint32_t*)pTemp)
      {
        return -1;
      }
      AppRunFlashDestination += 4;
      pTemp += 4;
    }
  }
  return 0;
}









