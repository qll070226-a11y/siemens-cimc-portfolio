/* Reuse all validated preliminary interrupt handlers, replacing USART1 only. */
#define USART1_IRQHandler contest_legacy_usart1_irq
#include "gd32f4xx_it.c"
#undef USART1_IRQHandler

#include "contest_modbus.h"

void USART1_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(RS232_RS485_USART, USART_INT_FLAG_RBNE))
        contest_modbus_rx_isr();
    if (RESET != usart_interrupt_flag_get(RS232_RS485_USART, USART_INT_FLAG_TBE))
        contest_modbus_tx_isr();
}

void TIMER6_IRQHandler(void)
{
    if (SET == timer_interrupt_flag_get(TIMER6, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
        contest_modbus_timer_isr();
    }
}
