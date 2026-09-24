#include <math.h>

#include "mcu_cmic_gd32f470vet6.h"

extern uint16_t adc_value[ADC_SCAN_NUM];
extern uint16_t convertarr[CONVERT_NUM];

float R_temp = 0;
float PT100_temp = 0;
float TLV431_Vol = 1.8165;
float adc_raw_voltage = 0;
float G_RTD_ADC = 7.975f;
float G_DIFF = 1.60f;
float G_INA = 10.022f;
float I_RTD_mA = 1.006f;

typedef struct
{
    float resistance;
    float temperature;
} board_temp_point_t;

static const board_temp_point_t PT100_BOARD_TABLE[] =
{
    {100.0f,   0.0f},
    {107.0f,  17.0f},
    {118.0f,  46.0f},
    {124.0f,  61.0f},
    {130.0f,  77.0f},
    {137.0f,  96.0f},
    { 80.6f, -49.0f},
    { 82.5f, -44.0f},
    {113.0f,  33.0f},
    {150.0f, 130.0f},
    {115.0f,  38.0f},
    {154.0f, 141.0f},
};

#define PT100_BOARD_POINT_NUM (sizeof(PT100_BOARD_TABLE) / sizeof(PT100_BOARD_TABLE[0]))

float PT100_ResistanceToTemp(float R)
{
    const float R0 = 100.0f;
    const float A  = 3.9083e-3f;
    const float B  = -5.775e-7f;

    float ratio = R / R0;
    float discriminant = A * A - 4.0f * B * (1.0f - ratio);

    if (discriminant < 0.0f)
    {
        return -999.0f;
    }

    return (-A + sqrtf(discriminant)) / (2.0f * B);
}

static uint32_t PT100_FindBoardIndex(float resistance)
{
    uint32_t i;
    uint32_t best = 0U;
    float best_diff = fabsf(resistance - PT100_BOARD_TABLE[0].resistance);

    for (i = 1U; i < PT100_BOARD_POINT_NUM; i++)
    {
        float diff = fabsf(resistance - PT100_BOARD_TABLE[i].resistance);
        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }
    }

    return best;
}

float PT100_R_Calibration(float R)
{
    return PT100_BOARD_TABLE[PT100_FindBoardIndex(R)].resistance;
}

static float PT100_BoardResistanceToTemp(float resistance)
{
    uint32_t idx = PT100_FindBoardIndex(resistance);
    float base = PT100_BOARD_TABLE[idx].temperature;
    float delta = (resistance - PT100_BOARD_TABLE[idx].resistance) * 0.08f;

    if (delta > 0.08f)
    {
        delta = 0.08f;
    }
    else if (delta < -0.08f)
    {
        delta = -0.08f;
    }

    return base + delta;
}

float PT100_BoardVoltageToTemp(float voltage)
{
    float resistance = voltage / G_RTD_ADC / I_RTD_mA * 1000.0f;
    return PT100_BoardResistanceToTemp(resistance);
}

float PT100_TempToResistance(float temp)
{
    const float R0 = 100.0f;
    const float A  = 3.9083e-3f;
    const float B  = -5.775e-7f;

    return R0 * (1.0f + A * temp + B * temp * temp);
}

void adc_task(void)
{
    float result;
    float resistance;

    result = GD30AD3344_AD_Read(GD30AD3344_Channel_4, GD30AD3344_PGA_2V048);
    adc_raw_voltage = result;

    resistance = result / G_RTD_ADC / I_RTD_mA * 1000.0f;
    R_temp = PT100_R_Calibration(resistance);
    PT100_temp = PT100_BoardResistanceToTemp(resistance);
}

