#include "timeutil.h"

static int32_t days_from_civil(int32_t y, uint32_t m, uint32_t d)
{
    y -= (m <= 2);
    int32_t era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
    uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

static void civil_from_days(int32_t z, uint16_t *yy, uint8_t *mm, uint8_t *dd)
{
    z += 719468;
    int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    int32_t y = (int32_t)yoe + era * 400;
    uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    uint32_t mp = (5u * doy + 2u) / 153u;
    uint32_t d = doy - (153u * mp + 2u) / 5u + 1u;
    uint32_t m = mp + (mp < 10u ? 3u : -9u);
    y += (m <= 2);
    *yy = (uint16_t)y;
    *mm = (uint8_t)m;
    *dd = (uint8_t)d;
}

void epoch_to_datetime(uint32_t epoch, datetime_t *dt)
{
    uint32_t days = epoch / 86400u;
    uint32_t rem  = epoch % 86400u;
    dt->hour   = (uint8_t)(rem / 3600u);
    dt->minute = (uint8_t)((rem % 3600u) / 60u);
    dt->second = (uint8_t)(rem % 60u);
    dt->wday   = (uint8_t)((days + 4u) % 7u);
    civil_from_days((int32_t)days, &dt->year, &dt->month, &dt->day);
}

uint32_t datetime_to_epoch(const datetime_t *dt)
{
    int32_t days = days_from_civil((int32_t)dt->year, dt->month, dt->day);
    return (uint32_t)days * 86400u + dt->hour * 3600u + dt->minute * 60u + dt->second;
}

void time_format(uint32_t epoch, char *out)
{
    datetime_t dt;
    epoch_to_datetime(epoch, &dt);
    out[0]  = (char)('0' + (dt.year / 1000) % 10);
    out[1]  = (char)('0' + (dt.year / 100) % 10);
    out[2]  = (char)('0' + (dt.year / 10) % 10);
    out[3]  = (char)('0' + dt.year % 10);
    out[4]  = '-';
    out[5]  = (char)('0' + dt.month / 10);
    out[6]  = (char)('0' + dt.month % 10);
    out[7]  = '-';
    out[8]  = (char)('0' + dt.day / 10);
    out[9]  = (char)('0' + dt.day % 10);
    out[10] = ' ';
    out[11] = (char)('0' + dt.hour / 10);
    out[12] = (char)('0' + dt.hour % 10);
    out[13] = ':';
    out[14] = (char)('0' + dt.minute / 10);
    out[15] = (char)('0' + dt.minute % 10);
    out[16] = ':';
    out[17] = (char)('0' + dt.second / 10);
    out[18] = (char)('0' + dt.second % 10);
    out[19] = '\0';
}
