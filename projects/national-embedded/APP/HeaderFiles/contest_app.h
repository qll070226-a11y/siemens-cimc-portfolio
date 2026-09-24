#ifndef CONTEST_APP_H
#define CONTEST_APP_H

#include <stdint.h>

typedef struct {
    float current_ma;
    float voltage_1_v;
    float voltage_2_v;
    float adc_voltage[3];
    uint32_t sample_count;
    uint8_t wire_break;
    uint8_t tf_ready;
    uint8_t tf_error;
    uint8_t reserved;
} contest_measurement_t;

extern contest_measurement_t g_contest_measurement;

void contest_app_init(void);
void contest_sample_task(void);
void contest_storage_task(void);

#endif /* CONTEST_APP_H */
