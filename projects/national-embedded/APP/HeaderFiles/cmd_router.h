#ifndef CMD_ROUTER_H
#define CMD_ROUTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void cmd_router_init(void);

uint32_t cmd_router_handle(const char *rx_ascii, uint32_t rx_len,
                           char *out, uint32_t out_cap);

#ifdef __cplusplus
}
#endif

#endif /* CMD_ROUTER_H */
