#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXTFLASH_PARAM_ADDR    0x000000u
#define EXTFLASH_ALARM_ADDR    0x001000u

void storage_init(void);

int  storage_load_params(void);

void storage_save_params(void);

void storage_save_alarms(const void *buf, uint32_t len);
void storage_load_alarms(void *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_H */
