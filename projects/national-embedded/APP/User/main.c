#include "mcu_cmic_gd32f470vet6.h"

int main(void)
{
#ifdef __FIRMWARE_VERSION_DEFINE
    uint32_t fw_ver = 0;
#endif
    SCB->VTOR = FLASH_APP_ADDR;

    systick_config();
    init_cycle_counter(false);
    delay_ms(200); // SWIO 当 GPIO 用时，先等下载器接入

#ifdef __FIRMWARE_VERSION_DEFINE
    fw_ver = gd32f4xx_firmware_version_get();
#endif /* __FIRMWARE_VERSION_DEFINE */

    bsp_led_init();
    bsp_btn_init();
    bsp_oled_init();
    bsp_gd25qxx_init();
    bsp_usart_all_init();

    my_printf(DEBUG_USART, "BOOT: start\r\n");

    my_printf(DEBUG_USART, "BOOT: gd30 init...\r\n");
    bsp_gd30ad3344_init();
    my_printf(DEBUG_USART, "BOOT: gd30 done\r\n");

    my_printf(DEBUG_USART, "BOOT: adc init...\r\n");
    bsp_adc_init();
    my_printf(DEBUG_USART, "BOOT: adc done\r\n");

    my_printf(DEBUG_USART, "BOOT: dac init...\r\n");
    bsp_dac_init();
    my_printf(DEBUG_USART, "BOOT: dac done\r\n");

    my_printf(DEBUG_USART, "BOOT: rtc init...\r\n");
    bsp_rtc_init();
    my_printf(DEBUG_USART, "BOOT: rtc done\r\n");

    sd_fatfs_init();
    app_btn_init();

    my_printf(DEBUG_USART, "BOOT: oled init...\r\n");
    OLED_Init();
    my_printf(DEBUG_USART, "BOOT: oled done\r\n");

#if SD_FATFS_DEMO_ENABLE
    sd_fatfs_test();
#else
    my_printf(DEBUG_USART, "BOOT: sd_fatfs_test skipped (SD_FATFS_DEMO_ENABLE=0)\r\n");
#endif

    storage_load_params();
    g_params.alarm_active = 0;
    bsp_rs485_set_baud(baud_code_to_value(g_params.baud_code));
    comm_init();
    channel_dac_set(g_params.dac_code);
    report_init();
    alarm_init();
    ui_init();
    my_printf(DEBUG_USART, "BOOT: app ready, id=%04X baud=%lu\r\n",
              g_params.device_id, (unsigned long)baud_code_to_value(g_params.baud_code));
    comm_send_heartbeat();
    modbus_protocol_init();
    eMBEnable();
		
    scheduler_init();
    while(1) {
        scheduler_run();
    }
}

#ifdef GD_ECLIPSE_GCC
/* Eclipse GCC 下把 printf 重定向到串口。 */
int __io_putchar(int ch)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE));
    return ch;
}
#else
/* 把 printf 重定向到 USART0。 */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
#endif /* GD_ECLIPSE_GCC */
