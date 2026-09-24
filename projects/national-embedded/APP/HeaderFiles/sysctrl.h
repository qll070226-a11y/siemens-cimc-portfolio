#ifndef SYSCTRL_H
#define SYSCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

void sysctrl_request_reboot(void);
void sysctrl_request_upgrade(void);
int  sysctrl_reboot_pending(void);
void sysctrl_process(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSCTRL_H */
