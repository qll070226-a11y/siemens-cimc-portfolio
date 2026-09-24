/* 设备 ID、波特率和复位处理*/
#include "mcu_cmic_gd32f470vet6.h"   /* 复位、延时和备份寄存器接口*/
#include "ota_def.h"
#include "app_params.h"
#include "sysctrl.h"

static volatile uint8_t s_reboot  = 0;
static volatile uint8_t s_upgrade = 0;

void sysctrl_request_reboot(void)  { s_reboot = 1; }
void sysctrl_request_upgrade(void) { s_upgrade = 1; s_reboot = 1; }
int  sysctrl_reboot_pending(void)  { return s_reboot; }

void sysctrl_process(void)
{
    uint8_t was_upgrade;

    if (!s_reboot) return;
    s_reboot = 0;
    was_upgrade = s_upgrade;

    if (s_upgrade)
    {
        s_upgrade = 0;
        rcu_periph_clock_enable(RCU_PMU);
        pmu_backup_write_enable();
        RTC_BKP1 = OTA_FLAG_MAGIC;
        RTC_BKP2 = g_params.device_id;
        RTC_BKP3 = g_params.baud_code;
    }

    delay_ms(was_upgrade ? 1200U : 5U);
    NVIC_SystemReset();
}
