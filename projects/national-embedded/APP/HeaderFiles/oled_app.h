#ifndef __OLED_APP_H__
#define __OLED_APP_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int oled_printf(uint8_t x, uint8_t y, const char *format, ...);
void oled_task(void);
/* 项目自定义。 */

#ifdef __cplusplus
}
#endif

#endif


