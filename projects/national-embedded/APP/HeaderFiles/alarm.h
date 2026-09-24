#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_MAX  10u

typedef struct {
    uint32_t epoch;
    uint8_t  ch;          /* 通道号。 */
    uint8_t  rsv[3];
    float    threshold;
    float    value;
} alarm_record_t;

typedef struct {
    uint32_t       magic;
    uint8_t        count;
    uint8_t        rsv[3];
    alarm_record_t rec[ALARM_MAX];
    uint32_t       crc;
} alarm_store_t;

void     alarm_init(void);
void     alarm_check(void);
void     alarm_clear(void);
void     alarm_rearm(uint8_t ch);
uint32_t alarm_format_all(char *out, uint32_t cap);

void     alarm_add(uint32_t epoch, uint8_t ch, float threshold, float value);
uint8_t  alarm_count(void);
int      alarm_edge_trigger(uint8_t ch, int over);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_H */
