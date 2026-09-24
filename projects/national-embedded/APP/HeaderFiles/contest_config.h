#ifndef CONTEST_CONFIG_H
#define CONTEST_CONFIG_H

/* 现场改题优先只改本文件。0/1 宏不要同时打开两种 RS485 协议。 */
#define CONTEST_PROTOCOL_INITIAL        0U
#define CONTEST_PROTOCOL_MODBUS_RTU     1U
#define CONTEST_PROTOCOL_MODE           CONTEST_PROTOCOL_MODBUS_RTU

#define CONTEST_MODBUS_SLAVE_ADDRESS    1U
#define CONTEST_MODBUS_BAUDRATE         19200U
#define CONTEST_SAMPLE_PERIOD_MS        100U
#define CONTEST_LOG_PERIOD_MS           1000U
#define CONTEST_TF_LOG_ENABLE           1U
#define CONTEST_TF_LOG_PATH             "0:/CIMC_DATA.CSV"

/* GD30AD3344 物理通道。若 PCB 接线变化，只改这三个映射。 */
#define CONTEST_ADC_CH_CURRENT          1U
#define CONTEST_ADC_CH_VOLTAGE_1        0U
#define CONTEST_ADC_CH_VOLTAGE_2        2U

/* 电路标定：工程量 = ADC 端电压 * gain + offset。 */
#define CONTEST_CURRENT_GAIN_MA_PER_V   11.337849f
#define CONTEST_CURRENT_OFFSET_MA       (-1.398318f)
#define CONTEST_VOLTAGE1_GAIN           5.500000f
#define CONTEST_VOLTAGE1_OFFSET_V       0.000000f
#define CONTEST_VOLTAGE2_GAIN           5.749090f
#define CONTEST_VOLTAGE2_OFFSET_V       (-1.779327f)

/* 4-20 mA 断线阈值。留出噪声裕量，现场标定后可调整。 */
#define CONTEST_WIRE_BREAK_MA           3.500000f
#define CONTEST_WIRE_RECOVER_MA         3.800000f
#define CONTEST_WIRE_BREAK_DELAY_MS     1000U

#endif /* CONTEST_CONFIG_H */
