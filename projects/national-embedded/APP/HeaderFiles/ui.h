/* OLED 界面接口。 */
#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UI_TEAM_NUMBER
#define UI_TEAM_NUMBER "2026931391"
#endif

void ui_init(void);
void ui_task(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
