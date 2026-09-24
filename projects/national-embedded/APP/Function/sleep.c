/* µÍ¹¦ºÄÈë¿Ú*/
#include "mcu_cmic_gd32f470vet6.h"
#include "app_params.h"
#include "rtc_time.h"
#include "timeutil.h"
#include "comm.h"
#include "sleep.h"

static uint8_t s_pending = 0;

static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10u) << 4) | (v % 10u)); }

void sleep_request(void) { s_pending = 1; }
int  sleep_pending(void) { return s_pending; }

static void rtc_alarm_after(uint32_t seconds)
{
    rtc_alarm_struct a;
    datetime_t dt;
    uint32_t e = rtc_time_get_epoch() + seconds;

    epoch_to_datetime(e, &dt);

    a.alarm_mask      = RTC_ALARM_DATE_MASK;
    a.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    a.alarm_day       = bin2bcd(dt.day);
    a.alarm_hour      = bin2bcd(dt.hour);
    a.alarm_minute    = bin2bcd(dt.minute);
    a.alarm_second    = bin2bcd(dt.second);
    a.am_pm           = RTC_AM;

    rtc_alarm_disable(RTC_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    rtc_alarm_config(RTC_ALARM0, &a);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);

    exti_flag_clear(EXTI_17);
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(EXTI_17);
    nvic_irq_enable(RTC_Alarm_IRQn, 0, 0);
}

void sleep_process(void)
{
    if (!s_pending) return;
    s_pending = 0;

    rtc_alarm_after(10u);
    bsp_enter_deepsleep();

    bsp_rs485_set_baud(baud_code_to_value(g_params.baud_code));
    comm_send_str("instrument wakeup");
}
