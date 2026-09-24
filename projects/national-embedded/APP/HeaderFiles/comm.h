#ifndef COMM_H
#define COMM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void comm_init(void);
void comm_task(void);

void comm_send(const char *data, uint32_t len);
void comm_send_str(const char *s);

void comm_send_heartbeat(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_H */
