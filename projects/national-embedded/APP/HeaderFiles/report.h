#ifndef REPORT_H
#define REPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void report_init(void);
void report_start(void);
void report_stop(void);
int  report_is_active(void);

uint32_t report_build_frame(char *out, uint32_t cap);

void report_task(void);

#ifdef __cplusplus
}
#endif

#endif /* REPORT_H */
