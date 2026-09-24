#include "mcu_cmic_gd32f470vet6.h"   /* RTC 配置和读写接口*/
#include "timeutil.h"
#include "rtc_time.h"

extern rtc_parameter_struct rtc_initpara;
extern __IO uint32_t prescaler_a, prescaler_s;

static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10u) << 4) | (v % 10u)); }
static uint8_t bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10u) + (v & 0x0Fu)); }

uint32_t rtc_time_get_epoch(void)
{
    rtc_parameter_struct t;
    datetime_t dt;

    rtc_current_time_get(&t);
    dt.year   = (uint16_t)(2000u + bcd2bin((uint8_t)t.year));
    dt.month  = bcd2bin((uint8_t)t.month);
    dt.day    = bcd2bin((uint8_t)t.date);
    dt.hour   = bcd2bin((uint8_t)t.hour);
    dt.minute = bcd2bin((uint8_t)t.minute);
    dt.second = bcd2bin((uint8_t)t.second);
    dt.wday   = 0;
    return datetime_to_epoch(&dt);
}

void rtc_time_set_epoch(uint32_t epoch)
{
    datetime_t dt;
    uint8_t y2;

    epoch_to_datetime(epoch, &dt);
    if (dt.year < 2000u) dt.year = 2000u;
    y2 = (uint8_t)(dt.year - 2000u);

    rtc_initpara.factor_asyn   = prescaler_a;
    rtc_initpara.factor_syn    = prescaler_s;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm         = RTC_AM;
    rtc_initpara.year          = bin2bcd(y2);
    rtc_initpara.month         = bin2bcd(dt.month);
    rtc_initpara.date          = bin2bcd(dt.day);
    rtc_initpara.day_of_week   = (dt.wday == 0u) ? 7u : dt.wday;
    rtc_initpara.hour          = bin2bcd(dt.hour);
    rtc_initpara.minute        = bin2bcd(dt.minute);
    rtc_initpara.second        = bin2bcd(dt.second);

    rtc_init(&rtc_initpara);
}
