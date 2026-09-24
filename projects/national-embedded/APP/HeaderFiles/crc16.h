#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16_modbus(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */
