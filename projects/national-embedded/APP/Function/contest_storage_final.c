#include <stdio.h>
#include "mcu_cmic_gd32f470vet6.h"
#include "contest_config.h"
#include "contest_storage.h"

static FATFS s_fs;
static uint8_t s_mounted;

static long to_milli(float value)
{
    return (long)(value * 1000.0f + ((value >= 0.0f) ? 0.5f : -0.5f));
}

static unsigned long abs_milli_fraction(long value)
{
    long fraction = value % 1000L;
    return (unsigned long)((fraction < 0L) ? -fraction : fraction);
}

int contest_storage_init(void)
{
    FIL file;
    FRESULT result;
    UINT written = 0U;
    static const char header[] = "time_ms,current_mA,voltage1_V,voltage2_V,wire_break\r\n";

    if (disk_initialize(0) != RES_OK) return -1;
    result = f_mount(0, &s_fs);
    if (result != FR_OK) return -2;
    result = f_open(&file, CONTEST_TF_LOG_PATH, FA_OPEN_ALWAYS | FA_WRITE);
    if (result != FR_OK) return -3;
    s_mounted = 1U;
    if (f_size(&file) == 0U) {
        result = f_write(&file, header, sizeof(header) - 1U, &written);
        if ((result != FR_OK) || (written != sizeof(header) - 1U)) {
            (void)f_close(&file);
            return -4;
        }
    }
    result = f_close(&file);
    return (result == FR_OK) ? 0 : -5;
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
    long current = to_milli(current_ma);
    long voltage1 = to_milli(voltage_1_v);
    long voltage2 = to_milli(voltage_2_v);

    if (!s_mounted) return -1;
    length = snprintf(line, sizeof(line),
                      "%lu,%ld.%03lu,%ld.%03lu,%ld.%03lu,%u\r\n",
                      (unsigned long)timestamp_ms,
                      current / 1000L, abs_milli_fraction(current),
                      voltage1 / 1000L, abs_milli_fraction(voltage1),
                      voltage2 / 1000L, abs_milli_fraction(voltage2),
                      (unsigned int)wire_break);
    if ((length <= 0) || ((uint32_t)length >= sizeof(line))) return -2;

    result = f_open(&file, CONTEST_TF_LOG_PATH, FA_OPEN_ALWAYS | FA_WRITE);
    if (result != FR_OK) return -3;
    result = f_lseek(&file, f_size(&file));
    if (result == FR_OK) result = f_write(&file, line, (UINT)length, &written);
    if ((result == FR_OK) && (written == (UINT)length)) result = f_sync(&file);
    if (f_close(&file) != FR_OK && result == FR_OK) result = FR_DISK_ERR;
    return (result == FR_OK && written == (UINT)length) ? 0 : -4;
}
