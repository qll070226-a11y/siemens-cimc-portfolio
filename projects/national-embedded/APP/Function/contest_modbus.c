#include <string.h>
#include "mcu_cmic_gd32f470vet6.h"
#include "contest_app.h"
#include "contest_config.h"
#include "contest_modbus.h"
#include "mb.h"
#include "mbport.h"

static USHORT s_input[MB_INPUT_REGISTER_COUNT];
static USHORT s_holding[MB_HOLD_REGISTER_COUNT];

static uint16_t clamp_u16(float value)
{
    if (value <= 0.0f) return 0U;
    if (value >= 65535.0f) return 65535U;
    return (uint16_t)(value + 0.5f);
}

static uint16_t signed_millivolts(float voltage)
{
    float millivolts = voltage * 1000.0f;
    int16_t value;

    if (millivolts >= 32767.0f) return 0x7FFFU;
    if (millivolts <= -32768.0f) return 0x8000U;
    value = (int16_t)(millivolts + ((millivolts >= 0.0f) ? 0.5f : -0.5f));
    return (uint16_t)value;
}

void contest_modbus_init(void)
{
    s_holding[MB_HOLD_SAMPLE_PERIOD_MS] = CONTEST_SAMPLE_PERIOD_MS;
    s_holding[MB_HOLD_LOG_PERIOD_MS] = CONTEST_LOG_PERIOD_MS;
    s_holding[MB_HOLD_LOG_ENABLE] = CONTEST_TF_LOG_ENABLE;

    (void)eMBInit(MB_RTU,
                  CONTEST_MODBUS_SLAVE_ADDRESS,
                  0U,
                  CONTEST_MODBUS_BAUDRATE,
                  MB_PAR_NONE);
    (void)eMBEnable();
}

void contest_modbus_task(void)
{
    (void)eMBPoll();
}

void contest_modbus_update_measurements(void)
{
    uint16_t status = 0U;
    if (g_contest_measurement.wire_break) status |= 0x0001U;
    if (g_contest_measurement.tf_ready)   status |= 0x0002U;
    if (g_contest_measurement.tf_error)   status |= 0x0004U;

    s_input[MB_INPUT_CURRENT_MA_X1000] =
        clamp_u16(g_contest_measurement.current_ma * 1000.0f);
    s_input[MB_INPUT_VOLTAGE1_MV] =
        clamp_u16(g_contest_measurement.voltage_1_v * 1000.0f);
    s_input[MB_INPUT_VOLTAGE2_MV] =
        clamp_u16(g_contest_measurement.voltage_2_v * 1000.0f);
    s_input[MB_INPUT_STATUS] = status;
    s_input[MB_INPUT_SAMPLE_COUNT_HI] =
        (uint16_t)(g_contest_measurement.sample_count >> 16);
    s_input[MB_INPUT_SAMPLE_COUNT_LO] =
        (uint16_t)g_contest_measurement.sample_count;
    s_input[MB_INPUT_ADC0_MV_SIGNED] =
        signed_millivolts(g_contest_measurement.adc_voltage[0]);
    s_input[MB_INPUT_ADC1_MV_SIGNED] =
        signed_millivolts(g_contest_measurement.adc_voltage[1]);
    s_input[MB_INPUT_ADC2_MV_SIGNED] =
        signed_millivolts(g_contest_measurement.adc_voltage[2]);
    s_input[MB_INPUT_FW_SIGNATURE] =
        (uint16_t)(0xC000U |
                   (CONTEST_ADC_CH_CURRENT << 8) |
                   (CONTEST_ADC_CH_VOLTAGE_1 << 4) |
                   CONTEST_ADC_CH_VOLTAGE_2);
}

eMBErrorCode eMBRegInputCB(UCHAR *buffer, USHORT address, USHORT count)
{
    USHORT index;
    if ((address == 0U) || ((USHORT)(address - 1U + count) > MB_INPUT_REGISTER_COUNT)) {
        return MB_ENOREG;
    }
    index = (USHORT)(address - 1U);
    while (count-- > 0U) {
        *buffer++ = (UCHAR)(s_input[index] >> 8);
        *buffer++ = (UCHAR)s_input[index++];
    }
    return MB_ENOERR;
}

eMBErrorCode eMBRegHoldingCB(UCHAR *buffer, USHORT address, USHORT count,
                             eMBRegisterMode mode)
{
    USHORT index;
    if ((address == 0U) || ((USHORT)(address - 1U + count) > MB_HOLD_REGISTER_COUNT)) {
        return MB_ENOREG;
    }
    index = (USHORT)(address - 1U);
    while (count-- > 0U) {
        if (mode == MB_REG_READ) {
            *buffer++ = (UCHAR)(s_holding[index] >> 8);
            *buffer++ = (UCHAR)s_holding[index];
        } else {
            s_holding[index] = (USHORT)((USHORT)buffer[0] << 8) | buffer[1];
            buffer += 2;
        }
        index++;
    }
    return MB_ENOERR;
}

eMBErrorCode eMBRegCoilsCB(UCHAR *buffer, USHORT address, USHORT count,
                           eMBRegisterMode mode)
{
    (void)buffer; (void)address; (void)count; (void)mode;
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB(UCHAR *buffer, USHORT address, USHORT count)
{
    (void)buffer; (void)address; (void)count;
    return MB_ENOREG;
}

void contest_modbus_rx_isr(void)    { (void)pxMBFrameCBByteReceived(); }
void contest_modbus_tx_isr(void)    { (void)pxMBFrameCBTransmitterEmpty(); }
void contest_modbus_timer_isr(void) { (void)pxMBPortCBTimerExpired(); }
