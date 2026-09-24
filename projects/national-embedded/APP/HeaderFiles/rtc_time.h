/* RTC 时间接口。 */
#ifndef RTC_TIME_H
#define RTC_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t rtc_time_get_epoch(void);
void     rtc_time_set_epoch(uint32_t epoch);

#ifdef __cplusplus
}
#endif

#endif /* RTC_TIME_H */
