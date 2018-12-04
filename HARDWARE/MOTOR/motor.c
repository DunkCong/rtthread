#include "motor.h"


void Motor_init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOB时钟

    
    GPIO_Initure.Pin=GPIO_PIN_8 | GPIO_PIN_9;     //PB8、9
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;      
    GPIO_Initure.Pull=GPIO_NOPULL;        
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     			//高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);    

	  AERATE=0; 
	  DEFLATE=0;	
}	