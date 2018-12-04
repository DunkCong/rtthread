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
 ALIENTEK ������STM32F429������ʵ��57
 USB U��(Host)ʵ��-HAL�⺯����
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com  
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

////�������ȼ�
//#define START_TASK_PRIO			1
////�����ջ��С	
//#define START_STK_SIZE 			256  
////������
//TaskHandle_t StartTask_Handler;
////������
//void start_task(void *pvParameters);

////�������ȼ�
//#define MAIN_TASK_PRIO	2
////�����ջ��С	
//#define MAIN_STK_SIZE 	256  
////������
//TaskHandle_t Main_Handler;
////������
//void main_task(void *pvParameters);

////�������ȼ�
//#define USB_TASK_PRIO	3
////�����ջ��С	
//#define USB_STK_SIZE 	256  
////������
//TaskHandle_t Usb_Handler;
////������
//void usb_task(void *pvParameters);

//TimerHandle_t	OneShotTimer_Handle;			//���ζ�ʱ�����
//void OneShotCallback(TimerHandle_t xTimer);		//���ζ�ʱ���ص�����

USBH_HOST  USB_Host;
USB_OTG_CORE_HANDLE  USB_OTG_Core;
int air_pressure;
_Bool state;
FIL fil;        /* File object */

//�û�����������
//����ֵ:0,����
//       1,������
u8 USH_User_App(void)
{  
	u32 total,free,t=0,bw;
	u8 res=0,key_value,minute=0,time=0;
	FRESULT fr;
  char buf[50]={0};
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;	
	
	Show_Str(30,140,200,16,"�豸���ӳɹ�!.",16,0);	 
  	f_mount(fs[3],"3:",1); 	//���¹���U��
	res=exf_getfree("3:",&total,&free);
	if(res==0)
	{
		POINT_COLOR=BLUE;//��������Ϊ��ɫ	   
		LCD_ShowString(30,160,200,16,16,"FATFS OK!");	
		LCD_ShowString(30,180,200,16,16,"U Disk Total Size:     MB");	 
		LCD_ShowString(30,200,200,16,16,"U Disk  Free Size:     MB"); 	    
		LCD_ShowNum(174,180,total>>10,5,16);//��ʾU�������� MB
		LCD_ShowNum(174,200,free>>10,5,16);	
		
	}
	//fr=mf_open("3:time.txt",FA_READ | FA_WRITE | FA_OPEN_ALWAYS);	
	
	while(HCD_IsDeviceConnected(&USB_OTG_Core))//�豸���ӳɹ�
	{	
		key_value=KEY_Scan(0);
    if(key_value==KEY0_PRES)
		{
		  //����	
			t=0;
			state=0;
			DEFLATE=0; 
			AERATE=1; 
			fr = f_open(&fil, "3:��������.txt", FA_READ | FA_WRITE | FA_OPEN_APPEND);
			if(fr) return fr;			
		}
    else if(key_value==KEY1_PRES)
		{
		  //����
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
		  AERATE=0;     //ֹͣ����
      state=1;
			LCD_ShowNum(250,250,air_pressure,2,32);
			HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);				
			sprintf(buf,"����ʱ��:%d mS  20%02d-%02d-%02d %02d:%02d:%02d\r\n",t*10,RTC_DateStruct.Year,RTC_DateStruct.Month,RTC_DateStruct.Date,RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds);
			//mf_write((u8 *)buf,strlen(buf));
		  res=f_write(&fil,buf,strlen(buf),&bw);
			if(res) return res;
			if(RTC_TimeStruct.Minutes>minute || (RTC_TimeStruct.Minutes==0 && minute==59))
			{	
				//ÿ���ӱ���һ��
				minute=RTC_TimeStruct.Minutes;
				res=f_sync(&fil);
				if(res) return res;
			}
      //else printf("soure num: %d, written num: %d\n",strlen(buf),bw);
			Show_Str(100,350,380,16,(u8 *)buf,16,0);
			memset(buf,0,sizeof(buf));
			
			t=0;          //���¼�ʱ����		
		}
		
    if(state==1 && t>=300)
		{
			t=0;
		  if(DEFLATE==1)
			{	
				//����
				state=0;
				DEFLATE=0;    
				AERATE=1;				
			}			
			if(AERATE==0) DEFLATE=1;    //����	
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
	LED1=1;				//�ر�LED1 
 	AERATE=0;
	DEFLATE=1;
	f_mount(0,"3:",1); 	//ж��U��
	POINT_COLOR=RED;//��������Ϊ��ɫ	   
	Show_Str(30,140,200,16,"�豸������...",16,0);
	LCD_Fill(30,160,239,220,WHITE); 
	return res;
} 


int main(void)
{
	u32 t=0;
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;
  char buf[50];	
	
	Stm32_Clock_Init(384,25,2,8);   //����ʱ��,192Mhz   
	delay_init(192);                //��ʼ����ʱ����
	uart_init(115200);              //��ʼ��USART
	usmart_dev.init(96);
	LED_Init();					//��ʼ����LED���ӵ�Ӳ���ӿ�
	SDRAM_Init();				//��ʼ��SDRAM 
	LCD_Init();					//��ʼ��LCD 
	W25QXX_Init();				//��ʼ��W25Q256 
	PCF8574_Init();				//��ʼ��PCF8574
	MY_ADC_Init();
	Motor_init();
	RTC_Init();
	KEY_Init();
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		//��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ�� 
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
 	f_mount(fs[0],"0:",1); 		//����SD�� 
 	f_mount(fs[1],"1:",1); 		//����SPI FLASH. 
 	f_mount(fs[2],"2:",1); 		//����NAND FLASH. 
	POINT_COLOR=RED;      
 	while(font_init()) 				//����ֿ�
	{	    
		LCD_ShowString(60,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(60,50,240,66,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}
	Show_Str(30,50,200,16,"������STM32F4/F7������",16,0);				    	 
	Show_Str(30,70,200,16,"USB U��ʵ��",16,0);					    	 
	Show_Str(30,90,200,16,"2016��1��24��",16,0);	    	 
	Show_Str(30,110,200,16,"����ԭ��@ALIENTEK",16,0); 
	Show_Str(30,140,200,16,"�豸������...",16,0);			 		
	//��ʼ��USB����
  	USBH_Init(&USB_OTG_Core,USB_OTG_FS_CORE_ID,&USB_Host,&USBH_MSC_cb,&USR_cb);  
	
//    //������ʼ����
//    xTaskCreate((TaskFunction_t )start_task,            //������
//                (const char*    )"start_task",          //��������
//                (uint16_t       )START_STK_SIZE,        //�����ջ��С
//                (void*          )NULL,                  //���ݸ��������Ĳ���
//                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
//                (TaskHandle_t*  )&StartTask_Handler);   //������                
//    vTaskStartScheduler();          //�����������	
	
	
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

////��ʼ����������
//void start_task(void *pvParameters)
//{
//    taskENTER_CRITICAL();           //�����ٽ���
//    //����������ڶ�ʱ��
//	
//    //�������ζ�ʱ��
//	OneShotTimer_Handle=xTimerCreate((const char*			)"OneShotTimer",
//							         (TickType_t			)2000,
//							         (UBaseType_t			)pdFALSE,
//							         (void*					)2,
//							         (TimerCallbackFunction_t)OneShotCallback); //���ζ�ʱ��������2s(2000��ʱ�ӽ���)������ģʽ					  
//    //����������
//    xTaskCreate((TaskFunction_t )main_task,             
//                (const char*    )"main_task",           
//                (uint16_t       )MAIN_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )MAIN_TASK_PRIO,        
//                (TaskHandle_t*  )&Main_Handler);

//    //����USB����
//    xTaskCreate((TaskFunction_t )usb_task,             
//                (const char*    )"usb_task",           
//                (uint16_t       )USB_STK_SIZE,        
//                (void*          )NULL,                  
//                (UBaseType_t    )USB_TASK_PRIO,        
//                (TaskHandle_t*  )&Usb_Handler);
//								
//    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
//    taskEXIT_CRITICAL();            //�˳��ٽ���
//}

//void main_task(void *pvParameters)
//{
//  AERATE=1;         //����
//	while(1)
//	{	
//		air_pressure=(Get_Adc()*3.3/4096+0.04)*25-5;
//		if(air_pressure>=40)
//		{
//		  AERATE=0;     //ֹͣ����
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
//		vTaskDelay(20); //��ʱ20ms��Ҳ����10��ʱ�ӽ���
//	}	
//}	

////���ζ�ʱ���Ļص�����
//void OneShotCallback(TimerHandle_t xTimer)
//{
//	static u8 tmr2_num = 0;
//	tmr2_num++;		//���ڶ�ʱ��ִ�д�����1
//	LCD_ShowxNum(190,111,tmr2_num,3,16,0x80);  //��ʾ���ζ�ʱ��ִ�д���
//	LED1=!LED1;
//    printf("��ʱ��2���н���\r\n");
//}