#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "app_params.h"
#include "channel.h"
#include "rtc_time.h"
#include "comm.h"
#include "storage.h"
#include "timeutil.h"
#include "crc16.h"
#include "alarm.h"

#define ALARM_MAGIC 0x414C524Du   /* 告警参数标记*/

static alarm_record_t s_rec[ALARM_MAX];
static uint8_t s_count;
static uint8_t s_was_over[CH_NUM];
static uint8_t s_primed;

static void alarm_persist(void)
{
    alarm_store_t st;
    memset(&st, 0, sizeof(st));
    st.magic = ALARM_MAGIC;
    st.count = s_count;
    memcpy(st.rec, s_rec, sizeof(s_rec));
    st.crc = (uint32_t)crc16_modbus((const uint8_t *)&st, offsetof(alarm_store_t, crc));
    storage_save_alarms(&st, sizeof(st));
}

void alarm_init(void)
{
    alarm_store_t st;
    uint8_t i;

    storage_load_alarms(&st, sizeof(st));
    if (st.magic == ALARM_MAGIC && st.count <= ALARM_MAX &&
        st.crc == (uint32_t)crc16_modbus((const uint8_t *)&st, offsetof(alarm_store_t, crc)))
    {
        s_count = st.count;
        memcpy(s_rec, st.rec, sizeof(s_rec));
    }
    else
    {
        s_count = 0;
        memset(s_rec, 0, sizeof(s_rec));
        alarm_persist();
    }
    for (i = 0; i < CH_NUM; i++) s_was_over[i] = 0;
    s_primed = 0;
}

void alarm_add(uint32_t epoch, uint8_t ch, float thr, float val)
{
    if (s_count >= ALARM_MAX)
    {
        memmove(&s_rec[0], &s_rec[1], sizeof(alarm_record_t) * (ALARM_MAX - 1u));
        s_count = (uint8_t)(ALARM_MAX - 1u);
    }
    s_rec[s_count].epoch     = epoch;
    s_rec[s_count].ch        = ch;
    s_rec[s_count].threshold = thr;
    s_rec[s_count].value     = val;
    s_count++;
    alarm_persist();
}

uint8_t alarm_count(void) { return s_count; }

void alarm_clear(void)
{
    s_count = 0;
    memset(s_rec, 0, sizeof(s_rec));
    alarm_persist();
}

void alarm_rearm(uint8_t ch)
{
    if (ch < CH_NUM) s_was_over[ch] = 0;
}

int alarm_edge_trigger(uint8_t ch, int over)
{
    int trig;
    if (ch >= CH_NUM) return 0;
    trig = over && !s_was_over[ch];
    s_was_over[ch] = (uint8_t)(over ? 1 : 0);
    return trig;
}

uint32_t alarm_format_all(char *out, uint32_t cap)
{
    int i;
    uint32_t n = 0;

    if (s_count == 0)
    {
        if (cap >= 6u) { memcpy(out, "empty", 6); return 5u; }
        if (cap) out[0] = '\0';
        return 0;
    }

    for (i = (int)s_count - 1; i >= 0; i--)
    {
        char ts[24];
        int w;
        time_format(s_rec[i].epoch, ts);
        w = snprintf(out + n, cap - n, "%s | CH%u | %.2f | %.2f\n",
                     ts, (unsigned)s_rec[i].ch,
                     (double)s_rec[i].threshold, (double)s_rec[i].value);
        if (w < 0 || (uint32_t)w >= cap - n) break;
        n += (uint32_t)w;
    }
    return n;
}

void alarm_check(void)
{
    uint8_t ch;
    for (ch = 0; ch < CH_NUM; ch++)
    {
        float v   = channel_get_value(ch);
        float thr = channel_get_threshold(ch);
        int   over;

        if (thr == 0.0f) { s_was_over[ch] = 0; continue; }

        over = (v > thr);
        if (!s_primed) { s_was_over[ch] = (uint8_t)(over ? 1 : 0); continue; }

        if (alarm_edge_trigger(ch, over))
        {
            uint32_t e = rtc_time_get_epoch();
            alarm_add(e, ch, thr, v);
            if (g_params.alarm_active)
            {
                char line[48], ts[24];
                time_format(e, ts);
                snprintf(line, sizeof(line), "%s | CH%u | %.2f | %.2f\n",
                         ts, (unsigned)ch, (double)thr, (double)v);
                comm_send_str(line);
            }
        }
    }
    s_primed = 1;
}
