/* SysTick 延时和节拍*/

#include "gd32f4xx.h"
#include "systick.h"

volatile static uint32_t delay;

/* 配置 SysTick */
void systick_config(void)
{
    /* 配置 1ms SysTick 中断*/
    if(SysTick_Config(SystemCoreClock / 1000U)) {
        /* 配置失败，停在这里 */
        while(1) {
        }
    }
    /* 设置 SysTick 中断优先级*/
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/* 毫秒延时*/
void delay_1ms(uint32_t count)
{
    delay = count;

    while(0U != delay) {
    }
}

/* 延时计数递减*/
void delay_decrement(void)
{
    if(0U != delay) {
        delay--;
    }
}
