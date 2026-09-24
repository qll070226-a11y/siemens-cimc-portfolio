/* Ë¯ÃßÃüÁî½Ó¿Ú¡£ */
#ifndef SLEEP_H
#define SLEEP_H

#ifdef __cplusplus
extern "C" {
#endif

void sleep_request(void);
int  sleep_pending(void);
void sleep_process(void);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_H */
