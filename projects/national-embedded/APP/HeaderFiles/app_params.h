#ifndef APP_PARAMS_H
#define APP_PARAMS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_PARAMS_MAGIC   0x53494D31u
#define CH_NUM             3u            /* 三个通道。 */

#define BAUD_CODE_4800     0x11u
#define BAUD_CODE_9600     0x12u
#define BAUD_CODE_19200    0x13u
#define BAUD_CODE_115200   0x14u

#define REPORT_INT_1S      0x01u
#define REPORT_INT_3S      0x02u
#define REPORT_INT_5S      0x03u

typedef struct {
    uint32_t magic;
    uint16_t device_id;
    uint8_t  baud_code;
    uint8_t  report_int_code;
    uint8_t  alarm_active;
    uint8_t  _rsv;
    uint16_t dac_code;
    float    scale[CH_NUM];
    float    threshold[CH_NUM];
    uint32_t crc;
} app_params_t;

extern app_params_t g_params;

void app_params_set_defaults(void);

uint32_t app_params_compute_crc(const app_params_t *p);
int      app_params_is_valid(const app_params_t *p);

uint32_t baud_code_to_value(uint8_t code);
uint32_t report_code_to_ms(uint8_t code);
int      device_id_valid(uint16_t id);

#ifdef __cplusplus
}
#endif

#endif /* APP_PARAMS_H */
