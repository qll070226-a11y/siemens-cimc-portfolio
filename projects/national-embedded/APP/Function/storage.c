/* Flash 存储区*/
#include <string.h>
#include "mcu_cmic_gd32f470vet6.h"   /* SPI Flash 操作接口*/
#include "app_params.h"
#include "storage.h"

void storage_init(void)
{
}

int storage_load_params(void)
{
    app_params_t tmp;

    spi_flash_buffer_read((uint8_t *)&tmp, EXTFLASH_PARAM_ADDR, (uint16_t)sizeof(tmp));

    if (app_params_is_valid(&tmp))
    {
        g_params = tmp;
        return 1;
    }

    app_params_set_defaults();
    storage_save_params();
    return 0;
}

void storage_save_params(void)
{
    g_params.crc = app_params_compute_crc(&g_params);
    spi_flash_sector_erase(EXTFLASH_PARAM_ADDR);
    spi_flash_buffer_write((uint8_t *)&g_params, EXTFLASH_PARAM_ADDR, (uint16_t)sizeof(g_params));
}

void storage_save_alarms(const void *buf, uint32_t len)
{
    spi_flash_sector_erase(EXTFLASH_ALARM_ADDR);
    spi_flash_buffer_write((uint8_t *)buf, EXTFLASH_ALARM_ADDR, (uint16_t)len);
}

void storage_load_alarms(void *buf, uint32_t len)
{
    spi_flash_buffer_read((uint8_t *)buf, EXTFLASH_ALARM_ADDR, (uint16_t)len);
}
