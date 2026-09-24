#include <stdio.h>
#include "mcu_cmic_gd32f470vet6.h"
#include "contest_config.h"
#include "contest_storage.h"

static FATFS s_fs;
static uint8_t s_mounted;

int contest_storage_init(void)
{
    FIL file;
    FRESULT result;
    UINT written;
    static const char header[] = "time_ms,current_mA,voltage1_V,voltage2_V,wire_break\r\n";

    if (disk_initialize(0) != RES_OK) return -1;
    result = f_mount(0, &s_fs);
    if (result != FR_OK) return -2;
    s_mounted = 1U;
    result = f_open(&file, CONTEST_TF_LOG_PATH, FA_OPEN_ALWAYS | FA_WRITE);
    if (result != FR_OK) return -3;
    if (f_size(&file) == 0U) {
        result = f_write(&file, header, sizeof(header) - 1U, &written);
        if ((result != FR_OK) || (written != sizeof(header) - 1U)) {
            (void)f_close(&file);
            return -4;
        }
    }
    (void)f_close(&file);
    return 0;
}

int contest_storage_append_csv(uint32_t timestamp_ms, float current_ma,
                               float voltage_1_v, float voltage_2_v,
                               uint8_t wire_break)
{
    char line[128];
    FIL file;
    FRESULT result;
    UINT written = 0U;
    int length;

    if (!s_mounted) return -1;
    length = snprintf(line, sizeof(line), "%lu,%.3f,%.3f,%.3f,%u\r\n",
                      (unsigned long)timestamp_ms, current_ma,
                      voltage_1_v, voltage_2_v, (unsigned int)wire_break);
    if ((length <= 0) || ((uint32_t)length >= sizeof(line))) return -2;
    result = f_open(&file, CONTEST_TF_LOG_PATH, FA_OPEN_ALWAYS | FA_WRITE);
    if (result == FR_OK) result = f_lseek(&file, f_size(&file));
    if (result == FR_OK) result = f_write(&file, line, (UINT)length, &written);
    if ((result == FR_OK) && (written == (UINT)length)) result = f_sync(&file);
    (void)f_close(&file);
    return (result == FR_OK && written == (UINT)length) ? 0 : -3;
}
