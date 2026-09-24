/* 中断服务函数。 */

#include "gd32f4xx_it.h"
#include "main.h"
#include "mcu_cmic_gd32f470vet6.h"
#include "systick.h"
#include "string.h"

extern uint8_t usart0_rxbuffer[512];
extern uint8_t uart_dma_buffer[512];
extern uint8_t rx_flag;

/* NMI 异常入口。 */
void NMI_Handler(void)
{
    /* NMI 异常，停在这里。 */
    while(1) {
    }
}

/* 硬件错误异常入口。 */
void HardFault_Handler(void)
{
    /* 硬件错误，停在这里。 */
    while(1) {
    }
}

/* 内存管理异常入口。 */
void MemManage_Handler(void)
{
    /* 内存管理错误，停在这里。 */
    while(1) {
    }
}

/* 总线错误异常入口。 */
void BusFault_Handler(void)
{
    /* 总线错误，停在这里。 */
    while(1) {
    }
}

/* 用法错误异常入口。 */
void UsageFault_Handler(void)
{
    /* 用法错误，停在这里。 */
    while(1) {
    }
}

/* SVC 异常入口。 */
void SVC_Handler(void)
{
    /* SVC 异常，停在这里。 */
    while(1) {
    }
}

/* 调试监控异常入口。 */
void DebugMon_Handler(void)
{
    /* 调试异常，停在这里。 */
    while(1) {
    }
}

/* PendSV 异常入口。 */
void PendSV_Handler(void)
{
    /* PendSV 异常，停在这里。 */
    while(1) {
    }
}

/* 串口空闲中断入口。 */
void USART0_IRQHandler(void)
{
    uint32_t rx_len;
    uint32_t copy_len;

    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)){
        /* 清除串口空闲标志。 */
        usart_data_receive(USART0);
        dma_channel_disable(USART0_RX_DMA_PERIPH, USART0_RX_DMA_CHANNEL);
        
        /* 计算本次接收长度。 */
        rx_len = sizeof(usart0_rxbuffer) - dma_transfer_number_get(USART0_RX_DMA_PERIPH, USART0_RX_DMA_CHANNEL);
        if((rx_len > 0U) && (rx_len <= sizeof(usart0_rxbuffer))){
            copy_len = rx_len;
            if(copy_len >= sizeof(uart_dma_buffer)){
                copy_len = sizeof(uart_dma_buffer) - 1U;
            }
            memset(uart_dma_buffer, 0, sizeof(uart_dma_buffer));
            memcpy(uart_dma_buffer, usart0_rxbuffer, copy_len);
            rx_flag = 1;
        }
        memset(usart0_rxbuffer, 0, sizeof(usart0_rxbuffer));
        
        /* 关闭 DMA 后重新配置。 */
        dma_flag_clear(USART0_RX_DMA_PERIPH, USART0_RX_DMA_CHANNEL, DMA_FLAG_FTF);
        dma_transfer_number_config(USART0_RX_DMA_PERIPH, USART0_RX_DMA_CHANNEL, sizeof(usart0_rxbuffer));
        dma_channel_enable(USART0_RX_DMA_PERIPH, USART0_RX_DMA_CHANNEL);
    }
}

void SDIO_IRQHandler(void)
{
}

void EXTI0_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_0)) {
        exti_interrupt_flag_clear(EXTI_0);
    }
}

/* SysTick 中断入口。 */
void SysTick_Handler(void)
{
    delay_decrement();
}
