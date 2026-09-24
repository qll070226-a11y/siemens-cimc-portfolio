#include "mcu_cmic_gd32f470vet6.h"

extern float R_temp;
extern float PT100_temp;
extern float adc_raw_voltage;

/* OLED 底层接口*/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512];
  va_list arg;
  int len;

  va_start(arg, format);
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

static void oled_show_float_fixed(uint8_t x, uint8_t y, float value, uint8_t width, uint8_t precision)
{
    char format[8];
    char buffer[20];

    snprintf(format, sizeof(format), "%%%d.%df", width, precision);
    snprintf(buffer, sizeof(buffer), format, value);
    OLED_ShowStr(x, y, buffer, 8);
}

void oled_task(void)
{
    oled_printf(0, 0, "Vol:");
    oled_show_float_fixed(32, 0, adc_raw_voltage, 8, 4);
    oled_printf(0, 1, "uwTick:%lld", (long long)get_system_ms());
    oled_printf(0, 2, "Temp:");
    oled_show_float_fixed(40, 2, PT100_temp, 7, 2);
    oled_printf(0, 3, "R:");
    oled_show_float_fixed(16, 3, R_temp, 7, 2);
}

/* 项目自定义*/
