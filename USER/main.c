#include "sys.h"
#include "delay.h" 
#include "led.h"  
#include "usart.h" 
#include "lcd.h" 
#include "ltdc.h"   
#include "sdram.h"    
#include "malloc.h" 
#include "usmart.h"  
#include "w25qxx.h"  
#include "pcf8574.h"    
#include "sdio_sdcard.h"
#include "ff.h"  
#include "exfuns.h"  
#include "text.h"
#include "usbh_usr.h"
#include "adc.h"
#include "motor.h"
#include "rtc.h"
#include "key.h"
#include "fattester.h"
#include "string.h"
//#include "FreeRTOS.h"
//#include "task.h"
//#include "timers.h"

/************************************************
 ALIENTEK 阿波罗STM32F429开发板实验57
 USB U盘(Host)实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com  
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

////任务优先级
//#define START_TASK_PRIO			1
////任务堆栈大小	
//#define START_STK_SIZE 			256  
////任务句柄
//TaskHandle_t StartTask_Handler;
////任务函数
//void start_task(void *pvParameters);

////任务优先级
//#define MAIN_TASK_PRIO	2
////任务堆栈大小	
//#define MAIN_STK_SIZE 	256  
////任务句柄
//TaskHandle_t Main_Handler;
////任务函数
//void main_task(void *pvParameters);

////任务优先级
//#define USB_TASK_PRIO	3
////任务堆栈大小	
//#define USB_STK_SIZE 	256  
////任务句柄
//TaskHandle_t Usb_Handler;
////任务函数
//void usb_task(void *pvParameters);

//TimerHandle_t	OneShotTimer_Handle;			//单次定时器句柄
//void OneShotCallback(TimerHandle_t xTimer);		//单次定时器回调函数

USBH_HOST  USB_Host;
USB_OTG_CORE_HANDLE  USB_OTG_Core;
int air_pressure;
_Bool state;
FIL fil;        /* File object */

//用户测试主程序
//返回值:0,正常
//       1,有问题
u8 USH_User_App(void)
{  
	u32 total,free,t=0,bw;
	u8 res=0,key_value,minute=0,time=0;
	FRESULT fr;
  char buf[50]={0};
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;	
	
	Show_Str(30,140,200,16,"设备连接成功!.",16,0);	 
  	f_mount(fs[3],"3:",1); 	//重新挂载U盘
	res=exf_getfree("3:",&total,&free);
	if(res==0)
	{
		POINT_COLOR=BLUE;//设置字体为蓝色	   
		LCD_ShowString(30,160,200,16,16,"FATFS OK!");	
		LCD_ShowString(30,180,200,16,16,"U Disk Total Size:     MB");	 
		LCD_ShowString(30,200,200,16,16,"U Disk  Free Size:     MB"); 	    
		LCD_ShowNum(174,180,total>>10,5,16);//显示U盘总容量 MB
		LCD_ShowNum(174,200,free>>10,5,16);	
		
	}
	//fr=mf_open("3:time.txt",FA_READ | FA_WRITE | FA_OPEN_ALWAYS);	
	
	while(HCD_IsDeviceConnected(&USB_OTG_Core))//设备连接成功
	{	
		key_value=KEY_Scan(0);
    if(key_value==KEY0_PRES)
		{
		  //充气	
			t=0;
			state=0;
			DEFLATE=0; 
			AERATE=1; 
			fr = f_open(&fil, "3:充气程序.txt", FA_READ | FA_WRITE | FA_OPEN_APPEND);
			if(fr) return fr;			
		}
    else if(key_value==KEY1_PRES)
		{
		  //放气
			state=0;
			AERATE=0;
      DEFLATE=1;
			memset(buf,0,sizeof(buf));
			f_lseek(&fil,0);
			res=f_read(&fil,buf,sizeof(buf),&bw);
			if(res) return res;
			//else printf("read date :%s,length :%d\n",buf,bw);
			f_close(&fil);		
		}	
		if(t%20==0) air_pressure=(Get_Adc_Average(10)*3.3/4096)*50/3-5;
		if(air_pressure>=40 && state==0)
		{
		  AERATE=0;     //停止充气
      state=1;
			LCD_ShowNum(250,250,air_pressure,2,32);
			HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);				
			sprintf(buf,"充气时间:%d mS  20%02d-%02d-%02d %02d:%02d:%02d\r\n",t*10,RTC_DateStruct.Year,RTC_DateStruct.Month,RTC_DateStruct.Date,RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds);
			//mf_write((u8 *)buf,strlen(buf));
		  res=f_write(&fil,buf,strlen(buf),&bw);
			if(res) return res;
			if(RTC_TimeStruct.Minutes>minute || (RTC_TimeStruct.Minutes==0 && minute==59))
			{	
				//每分钟保存一次
				minute=RTC_TimeStruct.Minutes;
				res=f_sync(&fil);
				if(res) return res;
			}
      //else printf("soure num: %d, written num: %d\n",strlen(buf),bw);
			Show_Str(100,350,380,16,(u8 *)buf,16,0);
			memset(buf,0,sizeof(buf));
			
			t=0;          //重新计时三秒		
		}
		
    if(state==1 && t>=300)
		{
			t=0;
		  if(DEFLATE==1)
			{	
				//充气
				state=0;
				DEFLATE=0;    
				AERATE=1;				
			}			
			if(AERATE==0) DEFLATE=1;    //放气	
		}
		
		t++;
		time++;
		delay_ms(10);
		if(t%20==0) 
		{	
			LCD_ShowNum(250,250,air_pressure,2,32);
			LED1=!LED1;
		}
		if(time%100==0)
		{
			time=0;
			HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);
			sprintf(buf,"20%02d-%02d-%02d %02d:%02d:%02d\r\n",RTC_DateStruct.Year,RTC_DateStruct.Month,RTC_DateStruct.Date,RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds);	
			LCD_ShowString(100,300,320,16,16,(u8 *)buf);
		}		
	}
	LED1=1;				//关闭LED1 
 	AERATE=0;
	DEFLATE=1;
	f_mount(0,"3:",1); 	//卸载U盘
	POINT_COLOR=RED;//设置字体为红色	   
	Show_Str(30,140,200,16,"设备连接中...",16,0);
	LCD_Fill(30,160,239,220,WHITE); 
	return res;
} 


int main(void)
{
	u32 t=0;
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;
  char buf[50];	
	
	Stm32_Clock_Init(384,25,2,8);   //设置时钟,192Mhz   
	delay_init(192);                //初始化延时函数
	uart_init(115200);              //初始化USART
	usmart_dev.init(96);
	LED_Init();					//初始化与LED连接的硬件接口
	SDRAM_Init();				//初始化SDRAM 
	LCD_Init();					//初始化LCD 
	W25QXX_Init();				//初始化W25Q256 
	PCF8574_Init();				//初始化PCF8574
	MY_ADC_Init();
	Motor_init();
	RTC_Init();
	KEY_Init();
 	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部内存池
	my_mem_init(SRAMCCM);		//初始化CCM内存池 
	exfuns_init();				//为fatfs相关变量申请内存  
 	f_mount(fs[0],"0:",1); 		//挂载SD卡 
 	f_mount(fs[1],"1:",1); 		//挂载SPI FLASH. 
 	f_mount(fs[2],"2:",1); 		//挂载NAND FLASH. 
	POINT_COLOR=RED;      
 	while(font_init()) 				//检查字库
	{	    
		LCD_ShowString(60,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(60,50,240,66,WHITE);//清除显示	     
		delay_ms(200);				  
	}
	Show_Str(30,50,200,16,"阿波罗STM32F4/F7开发板",16,0);				    	 
	Show_Str(30,70,200,16,"USB U盘实验",16,0);					    	 
	Show_Str(30,90,200,16,"2016年1月24日",16,0);	    	 
	Show_Str(30,110,200,16,"正点原子@ALIENTEK",16,0); 
	Show_Str(30,140,200,16,"设备连接中...",16,0);			 		
	//初始化USB主机
  	USBH_Init(&USB_OTG_Core,USB_OTG_FS_CORE_ID,&USB_Host,&USBH_MSC_cb,&USR_cb);  
	
//    //创建开始任务
//    xTaskCreate((TaskFunction_t )start_task,            //任务函数
//                (const char*    )"start_task",          //任务名称
//                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
//                (void*          )NULL,                  //传递给任务函数的参数
//                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
//                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄                
//    vTaskStartScheduler();          //开启任务调度	
	
	
	while(1)
	{		
		USBH_Process(&USB_OTG_Core, &USB_Host);		
		delay_ms(10);
		t++;
		if(t%100==0)
		{
			t=0;
			LED0=!LED0;
			HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);	
			sprintf(buf,"20%02d-%02d-%02d %02d:%02d:%02d\r\n",RTC_DateStruct.Year,RTC_DateStruct.Month,RTC_DateStruct.Date,RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds);
			LCD_ShowString(100,300,320,16,16,(u8 *)buf);
		}
		
	}	
}

////开始任务任务函数
//void start_task(void *pvParameters)
//{
//    taskENTER_CRITICAL();           //进入临界区
//    //创建软件周期定时器
//	
//    //创建单次定时器
//	OneShotTimer_Handle=xTimerCreate((const char*			)"OneShotTimer",
//							         (TickType_t			)2000,
//							         (UBaseType_t			)pdFALSE,
//							         (void*					)2,
//							         (TimerCallbackFunction_t)OneShotCallback); //单次定时器，周期2s(2000个时钟节拍)，单次模式					  
//    //创建主任务
//    xTaskCreate((TaskFunction_t )main_task,             
//                (const char*    )"main_task",           
//                (uint16_t       )MAIN_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )MAIN_TASK_PRIO,        
//                (TaskHandle_t*  )&Main_Handler);

//    //创建USB任务
//    xTaskCreate((TaskFunction_t )usb_task,             
//                (const char*    )"usb_task",           
//                (uint16_t       )USB_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )USB_TASK_PRIO,        
//                (TaskHandle_t*  )&Usb_Handler);
//								
//    vTaskDelete(StartTask_Handler); //删除开始任务
//    taskEXIT_CRITICAL();            //退出临界区
//}

//void main_task(void *pvParameters)
//{
//  AERATE=1;         //充气
//	while(1)
//	{	
//		air_pressure=(Get_Adc()*3.3/4096+0.04)*25-5;
//		if(air_pressure>=40)
//		{
//		  AERATE=0;     //停止充气
//		}
//		LED0=!LED0;
//    vTaskDelay(1000); 	
//	}

//}

//void usb_task(void *pvParameters)
//{
//  while(1)
//	{
//	  USBH_Process(&USB_OTG_Core, &USB_Host);
//		vTaskDelay(20); //延时20ms，也就是10个时钟节拍
//	}	
//}	

////单次定时器的回调函数
//void OneShotCallback(TimerHandle_t xTimer)
//{
//	static u8 tmr2_num = 0;
//	tmr2_num++;		//周期定时器执行次数加1
//	LCD_ShowxNum(190,111,tmr2_num,3,16,0x80);  //显示单次定时器执行次数
//	LED1=!LED1;
//    printf("定时器2运行结束\r\n");
//}