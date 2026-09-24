#ifndef CONTEST_MODBUS_H
#define CONTEST_MODBUS_H

#include <stdint.h>

enum {
    MB_INPUT_CURRENT_MA_X1000 = 0,
    MB_INPUT_VOLTAGE1_MV      = 1,
    MB_INPUT_VOLTAGE2_MV      = 2,
    MB_INPUT_STATUS           = 3,
    MB_INPUT_SAMPLE_COUNT_HI  = 4,
    MB_INPUT_SAMPLE_COUNT_LO  = 5,
    MB_INPUT_ADC0_MV_SIGNED   = 6,
    MB_INPUT_ADC1_MV_SIGNED   = 7,
    MB_INPUT_ADC2_MV_SIGNED   = 8,
    MB_INPUT_FW_SIGNATURE     = 9,
    MB_INPUT_REGISTER_COUNT   = 16
};

enum {
    MB_HOLD_SAMPLE_PERIOD_MS = 0,
    MB_HOLD_LOG_PERIOD_MS    = 1,
    MB_HOLD_LOG_ENABLE       = 2,
    MB_HOLD_REGISTER_COUNT   = 16
};

void contest_modbus_init(void);
void contest_modbus_task(void);
void contest_modbus_update_measurements(void);

void contest_modbus_rx_isr(void);
void contest_modbus_tx_isr(void);
void contest_modbus_timer_isr(void);

#endif /* CONTEST_MODBUS_H */
