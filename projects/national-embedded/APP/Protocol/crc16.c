/* CRC16-MODBUS implementation*/
#include "crc16.h"

uint16_t crc16_modbus(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    uint32_t i;
    uint8_t  j;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}
