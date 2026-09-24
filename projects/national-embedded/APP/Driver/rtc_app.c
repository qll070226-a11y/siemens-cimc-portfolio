#include "mcu_cmic_gd32f470vet6.h"

extern rtc_parameter_struct rtc_initpara;

/* 读取当前 RTC 时间*/
void rtc_task(void)
{
    rtc_current_time_get(&rtc_initpara);
}
