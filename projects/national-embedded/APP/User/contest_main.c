#include "mcu_cmic_gd32f470vet6.h"
#include "contest_app.h"
#include "contest_config.h"
#include "contest_modbus.h"

typedef struct { void (*run)(void); uint32_t period_ms; uint32_t last_ms; } contest_task_t;
static contest_task_t s_tasks[] = {
    {contest_modbus_task, 1U, 0U},
    {contest_sample_task, CONTEST_SAMPLE_PERIOD_MS, 0U},
    {contest_storage_task, CONTEST_LOG_PERIOD_MS, 0U},
    {ui_task, 50U, 0U}
};

static void contest_scheduler_run(void)
{
    uint32_t now = get_system_ms();
    uint32_t i;
    for (i = 0U; i < sizeof(s_tasks) / sizeof(s_tasks[0]); ++i) {
        if ((uint32_t)(now - s_tasks[i].last_ms) >= s_tasks[i].period_ms) {
            s_tasks[i].last_ms = now;
            s_tasks[i].run();
        }
    }
}

int main(void)
{
    SCB->VTOR = FLASH_APP_ADDR;
    systick_config();
    init_cycle_counter(false);
    delay_ms(200U);
    bsp_led_init(); bsp_btn_init(); bsp_oled_init(); bsp_gd25qxx_init();
    bsp_usart_all_init(); bsp_gd30ad3344_init(); bsp_adc_init();
    bsp_dac_init(); bsp_rtc_init(); sd_fatfs_init(); app_btn_init(); OLED_Init();
    storage_load_params();
    g_params.alarm_active = 0U;
    channel_dac_set(g_params.dac_code);
    alarm_init(); ui_init(); contest_app_init();
    my_printf(DEBUG_USART, "CIMC template ready: Modbus=%u baud=%lu\r\n",
              (unsigned int)CONTEST_MODBUS_SLAVE_ADDRESS,
              (unsigned long)CONTEST_MODBUS_BAUDRATE);
    while (1) contest_scheduler_run();
}

#ifndef GD_ECLIPSE_GCC
int fputc(int ch, FILE *stream)
{
    (void)stream;
    usart_data_transmit(DEBUG_USART, (uint8_t)ch);
    while (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TBE)) { }
    return ch;
}
#endif
