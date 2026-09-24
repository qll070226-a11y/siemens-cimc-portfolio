/* 协议帧格式和命令字。 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROTO_HEADER        0xA5B6u
#define PROTO_TAIL          0xB6A5u
#define PROTO_VERSION       0x02u
#define ID_BROADCAST        0xFFFFu
#define ID_UNASSIGNED       0x0000u

#define FRAME_CMD           0x01u
#define FRAME_ACK           0x02u
#define FRAME_HEARTBEAT     0x05u
#define FRAME_ERROR         0xFFu

#define ACK_OK              0xFFu

/* 系统命令字。 */
#define CMD_REBOOT          0x0101u
#define CMD_FACTORY_RESET   0x0102u
#define CMD_DEV_INFO        0x0103u
#define CMD_VER_QUERY       0x0104u
#define CMD_TIME_SET        0x0105u
#define CMD_TIME_QUERY      0x0106u
#define CMD_ID_SET          0x01A1u
#define CMD_BAUD_SET        0x01A2u
#define CMD_ID_QUERY        0x0111u
#define CMD_BAUD_QUERY      0x0112u

/* 数据命令字。 */
#define CMD_CH0_QUERY       0x0201u
#define CMD_CH1_QUERY       0x0202u
#define CMD_CH2_QUERY       0x0221u
#define CMD_CH0_SCALE_SET   0x0241u
#define CMD_CH1_SCALE_SET   0x0242u
#define CMD_REPORT_INT_SET  0x0261u

/* 控制命令字。 */
#define CMD_DAC_SET         0x0301u
#define CMD_AUTO_REP_START  0x0302u
#define CMD_AUTO_REP_STOP   0x0303u
#define CMD_SLEEP           0x03AAu

/* 阈值命令字。 */
#define CMD_THLD_READ_ALL   0x0400u
#define CMD_CH0_THLD_READ   0x0401u
#define CMD_CH1_THLD_READ   0x0402u
#define CMD_CH2_THLD_READ   0x0403u
#define CMD_CH0_THLD_WRITE  0x0411u
#define CMD_CH1_THLD_WRITE  0x0412u
#define CMD_CH2_THLD_WRITE  0x0413u

/* 升级命令字。 */
#define CMD_UPGRADE_REQ     0x0501u
#define CMD_UPGRADE_PREPARE 0x0502u
#define CMD_UPGRADE_EXEC    0x0503u

/* 告警和日志命令字。 */
#define CMD_ALARM_MODE_SET  0x0601u
#define CMD_ALARM_QUERY     0x0602u
#define CMD_ALARM_CLEAR     0x0603u
#define CMD_LOG_QUERY       0x0604u
#define CMD_LOG_CLEAR       0x0605u

#define CMD_HEARTBEAT       0x8888u
#define CMD_BROADCAST_FIND  0xFFFFu
#define CMD_ERR_WORD        0xEEEEu

#define PROTO_MAX_PAYLOAD   64u
#define PROTO_OVERHEAD      13u
#define PROTO_MAX_FRAME     (PROTO_OVERHEAD + PROTO_MAX_PAYLOAD)
#define PROTO_MAX_ASCII     (PROTO_MAX_FRAME * 2u + 1u)

typedef enum {
    PROTO_OK = 0,
    PROTO_ERR_NOT_FOUND,
    PROTO_ERR_TOO_SHORT,
    PROTO_ERR_HEXCHAR,
    PROTO_ERR_LEN,
    PROTO_ERR_TAIL,
    PROTO_ERR_CRC,
} proto_status_t;

typedef struct {
    uint16_t dev_id;
    uint8_t  type;
    uint16_t cmd;
    uint8_t  len;
    uint8_t  version;
    uint8_t  payload[PROTO_MAX_PAYLOAD];
} proto_frame_t;

uint32_t hex_encode(const uint8_t *in, uint32_t len, char *out);
int32_t  hex_decode(const char *in, uint32_t ascii_len, uint8_t *out, uint32_t out_cap);

proto_status_t proto_parse_ascii(const char *ascii, uint32_t ascii_len,
                                 proto_frame_t *f, uint32_t *consumed);

uint32_t proto_build_ascii(uint16_t dev_id, uint8_t type, uint16_t cmd,
                           const uint8_t *payload, uint8_t len,
                           char *out_ascii, uint32_t out_cap);

uint32_t proto_build_ack_ok(uint16_t dev_id, uint16_t cmd, char *out, uint32_t out_cap);
uint32_t proto_build_error(uint16_t dev_id, char *out, uint32_t out_cap);
uint32_t proto_build_heartbeat(uint16_t dev_id, char *out, uint32_t out_cap);

typedef enum {
    PROTO_ACT_DISPATCH = 0,
    PROTO_ACT_RESPOND_ERROR,
    PROTO_ACT_DROP,
} proto_action_t;

proto_action_t proto_classify(const char *ascii, uint32_t ascii_len,
                              uint16_t my_id, proto_frame_t *f);

void     be_put_u16(uint8_t *p, uint16_t v);
void     be_put_u32(uint8_t *p, uint32_t v);
uint16_t be_get_u16(const uint8_t *p);
uint32_t be_get_u32(const uint8_t *p);
void     be_put_f32(uint8_t *p, float v);
float    be_get_f32(const uint8_t *p);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
