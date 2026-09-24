#include "app_params.h"
#include "crc16.h"
#include <stddef.h>

app_params_t g_params;

uint32_t app_params_compute_crc(const app_params_t *p)
{
    return (uint32_t)crc16_modbus((const uint8_t *)p, offsetof(app_params_t, crc));
}

int app_params_is_valid(const app_params_t *p)
{
    if (p->magic != APP_PARAMS_MAGIC) return 0;
    return (p->crc == app_params_compute_crc(p)) ? 1 : 0;
}

void app_params_set_defaults(void)
{
    uint8_t i;
    g_params.magic           = APP_PARAMS_MAGIC;
    g_params.device_id       = 0x0001u;
    g_params.baud_code       = BAUD_CODE_19200;
    g_params.report_int_code = REPORT_INT_1S;
    g_params.alarm_active    = 0u;
    g_params._rsv            = 0u;
    g_params.dac_code        = 0u;
    for (i = 0; i < CH_NUM; i++)
    {
        g_params.scale[i]     = 1.0f;
        g_params.threshold[i] = 0.0f;
    }
    g_params.crc = 0u;
}

uint32_t baud_code_to_value(uint8_t code)
{
    switch (code)
    {
    case BAUD_CODE_4800:   return 4800u;
    case BAUD_CODE_9600:   return 9600u;
    case BAUD_CODE_19200:  return 19200u;
    case BAUD_CODE_115200: return 115200u;
    default:               return 0u;
    }
}

uint32_t report_code_to_ms(uint8_t code)
{
    switch (code)
    {
    case REPORT_INT_1S: return 1000u;
    case REPORT_INT_3S: return 3000u;
    case REPORT_INT_5S: return 5000u;
    default:            return 0u;
    }
}

int device_id_valid(uint16_t id)
{
    return (id >= 0x0001u && id <= 0xFFFEu) ? 1 : 0;
}
