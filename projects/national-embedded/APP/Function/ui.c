/* OLED 状态显示*/
#include <string.h>
#include "mcu_cmic_gd32f470vet6.h"   /* OLED、LED 和系统时间接口*/
#include "report.h"
#include "ui.h"

static char    s_last_line2[20] = "";
static uint32_t s_last_sec = 0xFFFFFFFFu;

static void ui_line(uint8_t y, const char *s)
{
    char buf[22];
    uint8_t i = 0;
    while (s[i] && i < 20) { buf[i] = s[i]; i++; }
    while (i < 20) buf[i++] = ' ';
    buf[i] = '\0';
    OLED_ShowStr(0, y, buf, 8);
}

void ui_init(void)
{
    ui_line(0, UI_TEAM_NUMBER);
    s_last_line2[0] = '\0';
}

void ui_task(void)
{
    const char *state = report_is_active() ? "AutoSample" : "IDLE";
    uint32_t now = get_system_ms();
    uint32_t sec = now / 1000u;

    if (strcmp(state, s_last_line2) != 0)
    {
        ui_line(2, state);
        strncpy(s_last_line2, state, sizeof(s_last_line2) - 1);
        s_last_line2[sizeof(s_last_line2) - 1] = '\0';
    }

    if (sec != s_last_sec)
    {
        s_last_sec = sec;
        LED1_TOGGLE;
    }

    if (report_is_active()) { LED2_ON; }
    else                    { LED2_OFF; }
}
