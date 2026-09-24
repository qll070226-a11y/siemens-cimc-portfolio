#ifndef CONTEST_STORAGE_H
#define CONTEST_STORAGE_H

#include <stdint.h>

int contest_storage_init(void);
int contest_storage_append_csv(uint32_t timestamp_ms,
                               float current_ma,
                               float voltage_1_v,
                               float voltage_2_v,
                               uint8_t wire_break);

#endif /* CONTEST_STORAGE_H */
