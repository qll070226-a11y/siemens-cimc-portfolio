/* BootLoader 板级驱动接口。 */
#ifndef BL_BSP_H
#define BL_BSP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BootLoader 板级驱动接口。 */
extern uint8_t           bl_rx_buf[512];
extern volatile uint32_t bl_rx_len;
extern volatile uint8_t  bl_rx_flag;

void bl_uart_init(void);                                  
void bl_uart_send(const uint8_t *data, uint32_t len);     
void bl_uart_send_str(const char *s);
void bl_oled_show_bootloader(void);

/* BootLoader 板级驱动接口。 */
int  bl_check_upgrade_flag(void);

extern uint16_t bl_device_id;   
extern uint32_t bl_baud;        

#ifdef __cplusplus
}
#endif

#endif /* BL_BSP_H */
