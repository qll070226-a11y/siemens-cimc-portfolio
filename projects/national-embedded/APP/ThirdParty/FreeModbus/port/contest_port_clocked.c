#include "mcu_cmic_gd32f470vet6.h"
#include "mb.h"
#include "mbport.h"

static volatile eMBEventType s_event;
static volatile BOOL s_event_pending;

BOOL xMBPortEventInit(void) { s_event_pending = FALSE; return TRUE; }
BOOL xMBPortEventPost(eMBEventType event) { s_event = event; s_event_pending = TRUE; return TRUE; }
BOOL xMBPortEventGet(eMBEventType *event)
{
    if (!s_event_pending) return FALSE;
    *event = s_event; s_event_pending = FALSE; return TRUE;
}

BOOL xMBPortSerialInit(UCHAR port, ULONG baudrate, UCHAR data_bits, eMBParity parity)
{
    (void)port; (void)data_bits; (void)parity;
    usart_interrupt_disable(RS232_RS485_USART, USART_INT_IDLE);
    usart_dma_receive_config(RS232_RS485_USART, USART_RECEIVE_DMA_DISABLE);
    dma_channel_disable(DMA_RS_USART, DMA_RS_USART_CHANNEL_RX);
    usart_disable(RS232_RS485_USART);
    usart_baudrate_set(RS232_RS485_USART, baudrate);
    usart_enable(RS232_RS485_USART);
    RS485_CS_SET(0);
    return TRUE;
}

void vMBPortSerialEnable(BOOL rx_enable, BOOL tx_enable)
{
    if (tx_enable) {
        usart_interrupt_disable(RS232_RS485_USART, USART_INT_RBNE);
        RS485_CS_SET(1);
        usart_interrupt_enable(RS232_RS485_USART, USART_INT_TBE);
        return;
    }
    usart_interrupt_disable(RS232_RS485_USART, USART_INT_TBE);
    while (RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TC)) { }
    RS485_CS_SET(0);
    if (rx_enable) usart_interrupt_enable(RS232_RS485_USART, USART_INT_RBNE);
    else           usart_interrupt_disable(RS232_RS485_USART, USART_INT_RBNE);
}

BOOL xMBPortSerialPutByte(CHAR byte) { usart_data_transmit(RS232_RS485_USART, (uint8_t)byte); return TRUE; }
BOOL xMBPortSerialGetByte(CHAR *byte) { *byte = (CHAR)usart_data_receive(RS232_RS485_USART); return TRUE; }

BOOL xMBPortTimersInit(USHORT timeout_50us)
{
    timer_parameter_struct config;
    uint32_t timer_clock_hz;
    uint32_t period_us = (uint32_t)timeout_50us * 50U;

    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL2);
    rcu_periph_clock_enable(RCU_TIMER6);
    timer_deinit(TIMER6);
    timer_struct_para_init(&config);
    /* APB1 is HCLK/4; MUL2 makes TIMER6 HCLK/2. Count at exactly 1 MHz. */
    timer_clock_hz = SystemCoreClock / 2U;
    config.prescaler = timer_clock_hz / 1000000U - 1U;
    config.period = period_us - 1U;
    config.alignedmode = TIMER_COUNTER_EDGE;
    config.counterdirection = TIMER_COUNTER_UP;
    config.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER6, &config);
    timer_single_pulse_mode_config(TIMER6, TIMER_SP_MODE_SINGLE);
    nvic_irq_enable(TIMER6_IRQn, 2U, 0U);
    return TRUE;
}
void vMBPortTimersEnable(void)
{
    timer_disable(TIMER6); timer_counter_value_config(TIMER6, 0U);
    timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER6, TIMER_INT_UP); timer_enable(TIMER6);
}
void vMBPortTimersDisable(void)
{
    timer_interrupt_disable(TIMER6, TIMER_INT_UP);
    timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP); timer_disable(TIMER6);
}
void vMBPortClose(void) { }
