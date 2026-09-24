#include "mcu_cmic_gd32f470vet6.h"

__IO uint16_t tx_count = 0;
__IO uint8_t rx_flag = 0;
__IO uint8_t rs_rx_flag = 0;
__IO uint32_t rs_rx_len = 0;
uint8_t uart_dma_buffer[512] = {0};
uint8_t rs_dma_buffer[256] = {0};

extern float R_temp;
extern float PT100_temp;
extern float adc_raw_voltage;

int my_printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;
    // 初始化可变参数
    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
    
    for(tx_count = 0; tx_count < len; tx_count++){
        usart_data_transmit(usart_periph, buffer[tx_count]);
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
    }
    
    return len;
}

void uart_task(void)
{
    if(rx_flag) {
        my_printf(DEBUG_USART, "%s", uart_dma_buffer);
        memset(uart_dma_buffer, 0, sizeof(uart_dma_buffer));
        rx_flag = 0;
    }

    if(rs_rx_flag) {
        uint32_t len = rs_rx_len;
        rs_rx_flag = 0;

        my_printf(DEBUG_USART, "RS485 RX(%lu): ", (unsigned long)len);
        for(uint32_t i = 0; i < len; i++) {
            usart_data_transmit(DEBUG_USART, rs_dma_buffer[i]);
            while(RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TBE));
        }
        my_printf(DEBUG_USART, "\r\nRS485 HEX:");
        for(uint32_t i = 0; i < len; i++) {
            my_printf(DEBUG_USART, " %02X", rs_dma_buffer[i]);
        }
        my_printf(DEBUG_USART, "\r\n");
        memset(rs_dma_buffer, 0, sizeof(rs_dma_buffer));
        rs_rx_len = 0;
    }
}

void rs485_task(void)
{
    RS485_CS_SET(1);
    my_printf(RS232_RS485_USART, "Vol:%.4f,R:%.2f,T:%.2f\r\n", adc_raw_voltage, R_temp, PT100_temp);
    while(RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TC));
    RS485_CS_SET(0);
}
