#include "mcu_cmic_gd32f470vet6.h"
#include "contest_adc.h"

float contest_adc_read_voltage(uint8_t channel)
{
    GD30AD3344_Channel_TypeDef mux;

    switch (channel) {
    case 0U: mux = GD30AD3344_MUX_AIN0_GND; break;
    case 1U: mux = GD30AD3344_MUX_AIN1_GND; break;
    case 2U: mux = GD30AD3344_MUX_AIN2_GND; break;
    case 3U: mux = GD30AD3344_MUX_AIN3_GND; break;
    default: return 0.0f;
    }

    return GD30AD3344_AD_Read(mux, GD30AD3344_PGA_2V048);
}
