#include "motor.h"


void Motor_init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOB_CLK_ENABLE();           //����GPIOBʱ��

    
    GPIO_Initure.Pin=GPIO_PIN_8 | GPIO_PIN_9;     //PB8��9
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      
    GPIO_Initure.Pull=GPIO_NOPULL;        
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     			//����
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);    

	  AERATE=0; 
	  DEFLATE=0;	
}	