/* 通道采样和 PT100 换算*/
#include <math.h>
#include "mcu_cmic_gd32f470vet6.h"   /* 外部采样、AD3344 读取和 DAC 输出*/
#include "app_params.h"
#include "channel.h"

#define ADC_FULL_SCALE     4095.0f
#define ADC_VREF_V         3.3f

static float TLV431_Vol = 1.8165f;
static float G_DIFF     = 1.60f;
static float G_INA      = 10.022f;
static float I_RTD_mA   = 1.006f;

typedef struct { float measured; float actual; } r_cal_point_t;
static const r_cal_point_t R_CAL_TABLE[] = {
    { 81.036f,  80.7f }, { 82.557f,  82.2f }, {100.570f, 100.4f},
    {107.800f, 107.6f}, {112.890f, 113.0f}, {115.365f, 115.3f},
    {124.170f, 124.0f}, {130.070f, 129.8f}, {137.950f, 137.7f},
    {149.950f, 149.8f}, {154.910f, 154.6f},
};
#define R_CAL_NUM (sizeof(R_CAL_TABLE)/sizeof(R_CAL_TABLE[0]))

static float pt100_r_to_temp(float R)
{
    const float R0 = 100.0f, A = 3.9083e-3f, B = -5.775e-7f;
    float ratio = R / R0;
    float disc  = A * A - 4.0f * B * (1.0f - ratio);
    if (disc < 0.0f) return -999.0f;
    return (-A + sqrtf(disc)) / (2.0f * B);
}

static float pt100_r_calib(float R)
{
    uint32_t i;
    const r_cal_point_t *p1 = &R_CAL_TABLE[R_CAL_NUM - 2];
    const r_cal_point_t *p2 = &R_CAL_TABLE[R_CAL_NUM - 1];
    if (R_CAL_NUM < 2) return R;
    if (R <= R_CAL_TABLE[0].measured) { p1 = &R_CAL_TABLE[0]; p2 = &R_CAL_TABLE[1]; }
    else if (R >= R_CAL_TABLE[R_CAL_NUM - 1].measured) {
    }
    else {
        for (i = 0; i < R_CAL_NUM - 1; i++)
            if (R >= R_CAL_TABLE[i].measured && R <= R_CAL_TABLE[i + 1].measured)
            { p1 = &R_CAL_TABLE[i]; p2 = &R_CAL_TABLE[i + 1]; break; }
    }
    if (p2->measured == p1->measured) return p1->actual;
    return p1->actual + (R - p1->measured) * (p2->actual - p1->actual) / (p2->measured - p1->measured);
}


void channel_init(void)
{
}

float channel_get_raw(uint8_t ch)
{
    switch (ch)
    {
    case CH0:
        return (float)adc_value[ADC_IDX_CH0] / ADC_FULL_SCALE * ADC_VREF_V;
    case CH1:
        return (float)adc_value[ADC_IDX_CH1] / ADC_FULL_SCALE * ADC_VREF_V;
    case CH2: /* PT100 电阻温度*/
    {
        float v = GD30AD3344_AD_Read(GD30AD3344_Channel_4, GD30AD3344_PGA_2V048);
        return PT100_BoardVoltageToTemp(v);
    }
    default:
        return 0.0f;
    }
}

float channel_get_value(uint8_t ch)
{
    if (ch >= CH_NUM) return 0.0f;
    return channel_get_raw(ch) * g_params.scale[ch];
}

void channel_set_scale(uint8_t ch, float scale)
{
    if (ch < CH_NUM) g_params.scale[ch] = scale;
}
float channel_get_scale(uint8_t ch)
{
    return (ch < CH_NUM) ? g_params.scale[ch] : 1.0f;
}

void channel_set_threshold(uint8_t ch, float thr)
{
    if (ch < CH_NUM) g_params.threshold[ch] = thr;
}
float channel_get_threshold(uint8_t ch)
{
    return (ch < CH_NUM) ? g_params.threshold[ch] : 0.0f;
}

void channel_dac_set(uint16_t code)
{
    if (code > 4095u) code = 4095u;
    g_params.dac_code = code;
    bsp_dac_set(code);
}
uint16_t channel_dac_get(void)
{
    return g_params.dac_code;
}
