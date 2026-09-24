/* 串口收帧和应答*/
#include <string.h>
#include "mcu_cmic_gd32f470vet6.h"   /* 串口和 RS485 方向控制*/
#include "protocol.h"
#include "app_params.h"
#include "cmd_router.h"
#include "sleep.h"
#include "sysctrl.h"
#include "comm.h"

extern uint8_t rs_dma_buffer[256];
extern __IO uint8_t  rs_rx_flag;
extern __IO uint32_t rs_rx_len;

#define COMM_TX_BUF 600
static char s_tx[COMM_TX_BUF];
static char s_rx_ascii[513];
static char s_bin_tx[COMM_TX_BUF / 2];

void comm_init(void)
{
    cmd_router_init();
}

void comm_send(const char *data, uint32_t len)
{
    uint32_t i;
    RS485_CS_SET(1);
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(RS232_RS485_USART, (uint8_t)data[i]);
        while (RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TBE));
    }
    while (RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TC));
    RS485_CS_SET(0);
}

void comm_send_str(const char *s)
{
    comm_send(s, (uint32_t)strlen(s));
}

void comm_send_heartbeat(void)
{
    uint32_t n = proto_build_heartbeat(g_params.device_id, s_tx, sizeof(s_tx));
    if (n) comm_send(s_tx, n);
}

static uint8_t comm_is_binary_frame(const uint8_t *data, uint32_t len)
{
    return (len >= 2U && data[0] == 0xA5U && data[1] == 0xB6U) ? 1U : 0U;
}

static uint8_t comm_is_ascii_frame(const char *data, uint32_t len)
{
    return (len >= 4U &&
            (data[0] == 'A' || data[0] == 'a') && data[1] == '5' &&
            (data[2] == 'B' || data[2] == 'b') && data[3] == '6') ? 1U : 0U;
}

void comm_task(void)
{
    uint32_t rx_len, tx_len;
    uint8_t binary_rx;

    if (!rs_rx_flag) return;

    rx_len = rs_rx_len;
    rs_rx_flag = 0;

    binary_rx = comm_is_binary_frame(rs_dma_buffer, rx_len);
    if (binary_rx)
    {
        uint32_t ascii_len;
        if ((rx_len * 2U + 1U) > sizeof(s_rx_ascii)) return;
        ascii_len = hex_encode(rs_dma_buffer, rx_len, s_rx_ascii);
        s_rx_ascii[ascii_len] = '\0';
        tx_len = cmd_router_handle(s_rx_ascii, ascii_len, s_tx, sizeof(s_tx));
    }
    else
    {
        tx_len = cmd_router_handle((const char *)rs_dma_buffer, rx_len, s_tx, sizeof(s_tx));
    }

    if (tx_len)
    {
        if (binary_rx && comm_is_ascii_frame(s_tx, tx_len))
        {
            int32_t bin_len = hex_decode(s_tx, tx_len, (uint8_t *)s_bin_tx, sizeof(s_bin_tx));
            if (bin_len > 0) comm_send(s_bin_tx, (uint32_t)bin_len);
            else             comm_send(s_tx, tx_len);
        }
        else
        {
            comm_send(s_tx, tx_len);
        }
    }

    if (sleep_pending()) sleep_process();

    if (sysctrl_reboot_pending()) sysctrl_process();
}
