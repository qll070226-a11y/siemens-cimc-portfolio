/* 通道读写接口。 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { CH0 = 0, CH1 = 1, CH2 = 2 };

void  channel_init(void);

float channel_get_raw(uint8_t ch);

float channel_get_value(uint8_t ch);

void  channel_set_scale(uint8_t ch, float scale);
float channel_get_scale(uint8_t ch);

void  channel_set_threshold(uint8_t ch, float thr);
float channel_get_threshold(uint8_t ch);

void     channel_dac_set(uint16_t code);
uint16_t channel_dac_get(void);

#ifdef __cplusplus
}
#endif

#endif /* CHANNEL_H */
