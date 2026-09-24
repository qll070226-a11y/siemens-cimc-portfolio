#include "mcu_cmic_gd32f470vet6.h"
#include "contest_adc.h"
#include "contest_app.h"
#include "contest_config.h"
#include "contest_modbus.h"
#include "contest_storage.h"

contest_measurement_t g_contest_measurement;

static uint16_t s_wire_break_low_samples;

static float clamp_industrial_voltage(float voltage)
{
    if (voltage < 0.0f) {
        return 0.0f;
    }
    if (voltage > 10.0f) {
        return 10.0f;
    }
    return voltage;
}

void contest_app_init(void)
{
    memset(&g_contest_measurement, 0, sizeof(g_contest_measurement));
    s_wire_break_low_samples = 0U;

#if CONTEST_TF_LOG_ENABLE
    g_contest_measurement.tf_ready = (contest_storage_init() == 0) ? 1U : 0U;
    g_contest_measurement.tf_error = g_contest_measurement.tf_ready ? 0U : 1U;
#endif

#if CONTEST_PROTOCOL_MODE == CONTEST_PROTOCOL_MODBUS_RTU
    contest_modbus_init();
#endif
}

void contest_sample_task(void)
{
    float adc_physical[3];
    float adc_current;
    float adc_voltage1;
    float adc_voltage2;

    adc_physical[0] = contest_adc_read_voltage(0U);
    adc_physical[1] = contest_adc_read_voltage(1U);
    adc_physical[2] = contest_adc_read_voltage(2U);
    g_contest_measurement.adc_voltage[0] = adc_physical[0];
    g_contest_measurement.adc_voltage[1] = adc_physical[1];
    g_contest_measurement.adc_voltage[2] = adc_physical[2];

    adc_current = adc_physical[CONTEST_ADC_CH_CURRENT];
    adc_voltage1 = adc_physical[CONTEST_ADC_CH_VOLTAGE_1];
    adc_voltage2 = adc_physical[CONTEST_ADC_CH_VOLTAGE_2];
    g_contest_measurement.current_ma =
        adc_current * CONTEST_CURRENT_GAIN_MA_PER_V + CONTEST_CURRENT_OFFSET_MA;
    g_contest_measurement.voltage_1_v = clamp_industrial_voltage(
        adc_voltage1 * CONTEST_VOLTAGE1_GAIN + CONTEST_VOLTAGE1_OFFSET_V);
    g_contest_measurement.voltage_2_v = clamp_industrial_voltage(
        adc_voltage2 * CONTEST_VOLTAGE2_GAIN + CONTEST_VOLTAGE2_OFFSET_V);
    if (!g_contest_measurement.wire_break) {
        uint16_t confirm_samples =
            (uint16_t)((CONTEST_WIRE_BREAK_DELAY_MS + CONTEST_SAMPLE_PERIOD_MS - 1U) /
                       CONTEST_SAMPLE_PERIOD_MS);
        if (g_contest_measurement.current_ma < CONTEST_WIRE_BREAK_MA) {
            if (s_wire_break_low_samples < confirm_samples) {
                s_wire_break_low_samples++;
            }
            if (s_wire_break_low_samples >= confirm_samples) {
                g_contest_measurement.wire_break = 1U;
            }
        } else {
            s_wire_break_low_samples = 0U;
        }
    } else if (g_contest_measurement.current_ma >= CONTEST_WIRE_RECOVER_MA) {
        g_contest_measurement.wire_break = 0U;
        s_wire_break_low_samples = 0U;
    }
    g_contest_measurement.sample_count++;

#if CONTEST_PROTOCOL_MODE == CONTEST_PROTOCOL_MODBUS_RTU
    contest_modbus_update_measurements();
#endif
}

void contest_storage_task(void)
{
#if CONTEST_TF_LOG_ENABLE
    if (!g_contest_measurement.tf_ready) {
        return;
    }

    if (contest_storage_append_csv(get_system_ms(),
                                   g_contest_measurement.current_ma,
                                   g_contest_measurement.voltage_1_v,
                                   g_contest_measurement.voltage_2_v,
                                   g_contest_measurement.wire_break) != 0) {
        g_contest_measurement.tf_error = 1U;
    }
#endif
}
