#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;
    uint8_t  month;   /* 月。 */
    uint8_t  day;     /* 日。 */
    uint8_t  hour;    /* 时。 */
    uint8_t  minute;  /* 秒。 */
    uint8_t  second;  /* 秒。 */
    uint8_t  wday;
} datetime_t;

void     epoch_to_datetime(uint32_t epoch, datetime_t *dt);
uint32_t datetime_to_epoch(const datetime_t *dt);
void     time_format(uint32_t epoch, char *out);

#ifdef __cplusplus
}
#endif

#endif /* TIMEUTIL_H */
