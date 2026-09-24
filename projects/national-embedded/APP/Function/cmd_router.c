#include <string.h>
#include "protocol.h"
#include "app_params.h"
#include "channel.h"
#include "storage.h"
#include "rtc_time.h"
#include "report.h"
#include "alarm.h"
#include "sleep.h"
#include "sysctrl.h"
#include "cmd_router.h"

static const uint8_t FW_VERSION[4] = { 0x02, 0x00, 0x01, 0x00 };

void cmd_router_init(void)
{
    channel_init();
}

static uint32_t ack_float(uint16_t cmd, float v, char *out, uint32_t cap)
{
    uint8_t p[4];
    be_put_f32(p, v);
    return proto_build_ascii(g_params.device_id, FRAME_ACK, cmd, p, 4, out, cap);
}

static uint32_t ack_u32(uint16_t cmd, uint32_t v, char *out, uint32_t cap)
{
    uint8_t p[4];
    be_put_u32(p, v);
    return proto_build_ascii(g_params.device_id, FRAME_ACK, cmd, p, 4, out, cap);
}

static uint32_t ack_ok(uint16_t cmd, char *out, uint32_t cap)
{
    return proto_build_ack_ok(g_params.device_id, cmd, out, cap);
}

static uint32_t ack_err(char *out, uint32_t cap)
{
    return proto_build_error(g_params.device_id, out, cap);
}

static uint32_t handle_cmd(const proto_frame_t *f, char *out, uint32_t cap)
{
    switch (f->cmd)
    {
    case CMD_CH0_QUERY: return ack_float(f->cmd, channel_get_value(CH0), out, cap);
    case CMD_CH1_QUERY: return ack_float(f->cmd, channel_get_value(CH1), out, cap);
    case CMD_CH2_QUERY: return ack_float(f->cmd, channel_get_value(CH2), out, cap);

    case CMD_CH0_SCALE_SET:
        if (f->len < 4) return ack_err(out, cap);
        channel_set_scale(CH0, be_get_f32(f->payload));
        storage_save_params();
        return ack_ok(f->cmd, out, cap);
    case CMD_CH1_SCALE_SET:
        if (f->len < 4) return ack_err(out, cap);
        channel_set_scale(CH1, be_get_f32(f->payload));
        storage_save_params();
        return ack_ok(f->cmd, out, cap);

    case CMD_DAC_SET:
        if (f->len < 2) return ack_err(out, cap);
        channel_dac_set(be_get_u16(f->payload));
        storage_save_params();
        return ack_ok(f->cmd, out, cap);

    case CMD_THLD_READ_ALL: {
        uint8_t p[8];
        be_put_f32(p,     channel_get_threshold(CH0));
        be_put_f32(p + 4, channel_get_threshold(CH1));
        return proto_build_ascii(g_params.device_id, FRAME_ACK, f->cmd, p, 8, out, cap);
    }
    case CMD_CH0_THLD_READ: return ack_float(f->cmd, channel_get_threshold(CH0), out, cap);
    case CMD_CH1_THLD_READ: return ack_float(f->cmd, channel_get_threshold(CH1), out, cap);
    case CMD_CH2_THLD_READ: return ack_float(f->cmd, channel_get_threshold(CH2), out, cap);

    case CMD_CH0_THLD_WRITE:
        if (f->len < 4) return ack_err(out, cap);
        channel_set_threshold(CH0, be_get_f32(f->payload));
        alarm_rearm(CH0);
        storage_save_params();
        return ack_ok(f->cmd, out, cap);
    case CMD_CH1_THLD_WRITE:
        if (f->len < 4) return ack_err(out, cap);
        channel_set_threshold(CH1, be_get_f32(f->payload));
        alarm_rearm(CH1);
        storage_save_params();
        return ack_ok(f->cmd, out, cap);
    case CMD_CH2_THLD_WRITE:
        if (f->len < 4) return ack_err(out, cap);
        channel_set_threshold(CH2, be_get_f32(f->payload));
        alarm_rearm(CH2);
        storage_save_params();
        return ack_ok(f->cmd, out, cap);

    case CMD_REBOOT:
        sysctrl_request_reboot();
        return ack_ok(f->cmd, out, cap);

    case CMD_UPGRADE_REQ:
        sysctrl_request_upgrade();
        return ack_ok(f->cmd, out, cap);

    case CMD_ID_SET: {
        uint16_t newid;
        if (f->len < 2) return 0;
        newid = be_get_u16(f->payload);
        if (!device_id_valid(newid)) return 0;
        g_params.device_id = newid;
        storage_save_params();
        return ack_ok(f->cmd, out, cap);
    }
    case CMD_ID_QUERY: {
        uint8_t p[2];
        be_put_u16(p, g_params.device_id);
        return proto_build_ascii(g_params.device_id, FRAME_ACK, f->cmd, p, 2, out, cap);
    }

    case CMD_BAUD_SET:
        if (f->len < 1 || baud_code_to_value(f->payload[0]) == 0) return ack_err(out, cap);
        g_params.baud_code = f->payload[0];
        storage_save_params();
        sysctrl_request_reboot();
        return ack_ok(f->cmd, out, cap);
    case CMD_BAUD_QUERY: {
        uint8_t p = g_params.baud_code;
        return proto_build_ascii(g_params.device_id, FRAME_ACK, f->cmd, &p, 1, out, cap);
    }

    case CMD_VER_QUERY:
        return proto_build_ascii(g_params.device_id, FRAME_ACK, f->cmd, FW_VERSION, 4, out, cap);
    case CMD_TIME_SET:
        if (f->len < 4) return ack_err(out, cap);
        rtc_time_set_epoch(be_get_u32(f->payload));
        return ack_ok(f->cmd, out, cap);
    case CMD_TIME_QUERY:
        return ack_u32(f->cmd, rtc_time_get_epoch(), out, cap);

    case CMD_REPORT_INT_SET:
        if (f->len < 1 || report_code_to_ms(f->payload[0]) == 0) return ack_err(out, cap);
        g_params.report_int_code = f->payload[0];
        return ack_ok(f->cmd, out, cap);
    case CMD_AUTO_REP_START:
        report_start();
        return report_build_frame(out, cap);
    case CMD_AUTO_REP_STOP:
        report_stop();
        return ack_ok(f->cmd, out, cap);

    case CMD_SLEEP:
        sleep_request();
        return ack_ok(f->cmd, out, cap);

    case CMD_ALARM_MODE_SET:
        if (f->len < 1) return ack_err(out, cap);
        g_params.alarm_active = (f->payload[0] == 0x01u) ? 1u : 0u;
        return ack_ok(f->cmd, out, cap);
    case CMD_ALARM_QUERY:
        return alarm_format_all(out, cap);
    case CMD_ALARM_CLEAR:
        alarm_clear();
        return ack_ok(f->cmd, out, cap);

    default:
        return ack_err(out, cap);
    }
}

uint32_t cmd_router_handle(const char *rx_ascii, uint32_t rx_len,
                           char *out, uint32_t out_cap)
{
    proto_frame_t f;
    proto_action_t act = proto_classify(rx_ascii, rx_len, g_params.device_id, &f);

    if (act == PROTO_ACT_DROP)          return 0;
    if (act == PROTO_ACT_RESPOND_ERROR) return ack_err(out, out_cap);

    if (report_is_active() &&
        !(f.type == FRAME_CMD && f.cmd == CMD_AUTO_REP_STOP))
    {
        return 0;
    }

    /* 分发处理。 */
    switch (f.type)
    {
    case FRAME_CMD:
        return handle_cmd(&f, out, out_cap);

    case FRAME_HEARTBEAT:
        if (f.cmd == CMD_BROADCAST_FIND)
            return proto_build_heartbeat(g_params.device_id, out, out_cap);
        return 0;

    default:
        return ack_err(out, out_cap);
    }
}
