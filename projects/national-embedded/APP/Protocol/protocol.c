/* Protocol encode/decode helpers*/
#include "protocol.h"
#include "crc16.h"
#include <string.h>

void be_put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
void be_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}
uint16_t be_get_u16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
uint32_t be_get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}
void be_put_f32(uint8_t *p, float v)
{
    uint32_t u; memcpy(&u, &v, 4); be_put_u32(p, u);
}
float be_get_f32(const uint8_t *p)
{
    uint32_t u = be_get_u32(p); float v; memcpy(&v, &u, 4); return v;
}

static const char HEXTAB[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

uint32_t hex_encode(const uint8_t *in, uint32_t len, char *out)
{
    uint32_t i;
    for (i = 0; i < len; i++)
    {
        out[i * 2]     = HEXTAB[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = HEXTAB[in[i] & 0x0F];
    }
    return len * 2;
}

int32_t hex_decode(const char *in, uint32_t ascii_len, uint8_t *out, uint32_t out_cap)
{
    uint32_t i;
    if (ascii_len & 1u) return -1;
    if (ascii_len / 2u > out_cap) return -1;
    for (i = 0; i < ascii_len; i += 2)
    {
        int hi = hex_nibble(in[i]);
        int lo = hex_nibble(in[i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i / 2] = (uint8_t)((hi << 4) | lo);
    }
    return (int32_t)(ascii_len / 2u);
}

static int32_t find_header(const char *a, uint32_t len, uint32_t from)
{
    uint32_t i;
    if (len < 4) return -1;
    for (i = from; i + 4 <= len; i++)
    {
        if ((a[i] == 'A' || a[i] == 'a') && a[i+1] == '5' &&
            (a[i+2] == 'B' || a[i+2] == 'b') && a[i+3] == '6')
        {
            return (int32_t)i;
        }
    }
    return -1;
}

proto_status_t proto_parse_ascii(const char *ascii, uint32_t ascii_len,
                                 proto_frame_t *f, uint32_t *consumed)
{
    uint8_t  bytes[PROTO_MAX_FRAME];
    int32_t  s;
    uint32_t avail, prefix_ascii, total_bytes, total_ascii, crc_off, tail_off;
    uint8_t  len;
    uint16_t calc_crc, frame_crc, tail;

    s = find_header(ascii, ascii_len, 0);
    if (s < 0) return PROTO_ERR_NOT_FOUND;

    avail = ascii_len - (uint32_t)s;
    prefix_ascii = 9u * 2u;
    if (avail < prefix_ascii) return PROTO_ERR_TOO_SHORT;

    if (hex_decode(ascii + s, prefix_ascii, bytes, sizeof(bytes)) != 9) return PROTO_ERR_HEXCHAR;
    len = bytes[7];
    if (len > PROTO_MAX_PAYLOAD) return PROTO_ERR_LEN;

    total_bytes = 9u + len + 2u + 2u;
    total_ascii = total_bytes * 2u;
    if (avail < total_ascii) return PROTO_ERR_LEN;

    if (hex_decode(ascii + s, total_ascii, bytes, sizeof(bytes)) != (int32_t)total_bytes)
        return PROTO_ERR_HEXCHAR;

    crc_off  = 9u + len;
    tail_off = crc_off + 2u;

    tail = be_get_u16(bytes + tail_off);
    if (tail != PROTO_TAIL) return PROTO_ERR_TAIL;

    calc_crc  = crc16_modbus(bytes, crc_off);
    frame_crc = be_get_u16(bytes + crc_off);
    if (calc_crc != frame_crc) return PROTO_ERR_CRC;

    f->dev_id  = be_get_u16(bytes + 2);
    f->type    = bytes[4];
    f->cmd     = be_get_u16(bytes + 5);
    f->len     = len;
    f->version = bytes[8];
    if (len) memcpy(f->payload, bytes + 9, len);

    if (consumed) *consumed = (uint32_t)s + total_ascii;
    return PROTO_OK;
}

uint32_t proto_build_ascii(uint16_t dev_id, uint8_t type, uint16_t cmd,
                           const uint8_t *payload, uint8_t len,
                           char *out_ascii, uint32_t out_cap)
{
    uint8_t  bytes[PROTO_MAX_FRAME];
    uint32_t n = 0, total_bytes, ascii_len;
    uint16_t crc;

    if (len > PROTO_MAX_PAYLOAD) return 0;
    total_bytes = 9u + len + 2u + 2u;
    if (out_cap < total_bytes * 2u + 1u) return 0;

    be_put_u16(bytes + 0, PROTO_HEADER);
    be_put_u16(bytes + 2, dev_id);
    bytes[4] = type;
    be_put_u16(bytes + 5, cmd);
    bytes[7] = len;
    bytes[8] = PROTO_VERSION;
    if (len && payload) memcpy(bytes + 9, payload, len);

    crc = crc16_modbus(bytes, 9u + len);
    be_put_u16(bytes + 9 + len, crc);
    be_put_u16(bytes + 9 + len + 2, PROTO_TAIL);

    n = total_bytes;
    ascii_len = hex_encode(bytes, n, out_ascii);
    out_ascii[ascii_len] = '\0';
    return ascii_len;
}

uint32_t proto_build_ack_ok(uint16_t dev_id, uint16_t cmd, char *out, uint32_t out_cap)
{
    uint8_t p = ACK_OK;
    return proto_build_ascii(dev_id, FRAME_ACK, cmd, &p, 1, out, out_cap);
}

uint32_t proto_build_error(uint16_t dev_id, char *out, uint32_t out_cap)
{
    return proto_build_ascii(dev_id, FRAME_ERROR, CMD_ERR_WORD, NULL, 0, out, out_cap);
}

uint32_t proto_build_heartbeat(uint16_t dev_id, char *out, uint32_t out_cap)
{
    return proto_build_ascii(dev_id, FRAME_HEARTBEAT, CMD_HEARTBEAT, NULL, 0, out, out_cap);
}

proto_action_t proto_classify(const char *ascii, uint32_t ascii_len,
                              uint16_t my_id, proto_frame_t *f)
{
    proto_status_t st = proto_parse_ascii(ascii, ascii_len, f, NULL);

    switch (st)
    {
    case PROTO_OK:
        if (f->dev_id == ID_BROADCAST || f->dev_id == my_id)
            return PROTO_ACT_DISPATCH;
        return PROTO_ACT_DROP;

    case PROTO_ERR_CRC:
    case PROTO_ERR_LEN:
    case PROTO_ERR_TAIL:
    case PROTO_ERR_HEXCHAR:
        return PROTO_ACT_RESPOND_ERROR;

    case PROTO_ERR_NOT_FOUND:
    case PROTO_ERR_TOO_SHORT:
    default:
        return PROTO_ACT_DROP;
    }
}
