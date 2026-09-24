/* 定时上报组帧*/
#include "mcu_cmic_gd32f470vet6.h"   /* 系统时间*/
#include "protocol.h"
#include "app_params.h"
#include "channel.h"
#include "rtc_time.h"
#include "comm.h"
#include "report.h"

static uint8_t  s_active   = 0;
static uint32_t s_last_ms  = 0;
static char     s_buf[PROTO_MAX_ASCII];

void report_init(void)
{
    s_active = 0;
}

uint32_t report_build_frame(char *out, uint32_t cap)
{
    uint8_t p[12];
    be_put_u32(p + 0, rtc_time_get_epoch());
    be_put_f32(p + 4, channel_get_value(CH0));
    be_put_f32(p + 8, channel_get_value(CH1));
    return proto_build_ascii(g_params.device_id, FRAME_ACK, CMD_AUTO_REP_START, p, 12, out, cap);
}

void report_start(void)
{
    s_active  = 1;
    s_last_ms = get_system_ms();
}

void report_stop(void)
{
    s_active = 0;
}

int report_is_active(void)
{
    return s_active;
}

void report_task(void)
{
    uint32_t interval, now;

    if (!s_active) return;

    interval = report_code_to_ms(g_params.report_int_code);
    if (interval == 0) interval = 1000u;

    now = get_system_ms();
    if ((now - s_last_ms) >= interval)
    {
        s_last_ms = now;
        uint32_t n = report_build_frame(s_buf, sizeof(s_buf));
        if (n) comm_send(s_buf, n);
    }
}
