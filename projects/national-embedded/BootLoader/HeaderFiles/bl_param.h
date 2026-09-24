#ifndef COMMON_BL_PARAM_H
#define COMMON_BL_PARAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bl_partition.h"

/*
 * 参数页布局：主参数、备份参数、环形日志。
 * GD32 内部 Flash 按页擦除，所以参数变化时整页重写。
 */
#define BL_PARAM_PAGE_ADDR          0x0800C000UL
#define BL_PARAM_PAGE_SIZE          0x00001000UL
#define BL_PARAM_MAIN_ADDR          0x0800C000UL
#define BL_PARAM_BACKUP_ADDR        0x0800C100UL
#define BL_LOG_ADDR                 0x0800C200UL
#define BL_LOG_ENTRY_SIZE           32UL
#define BL_LOG_ENTRY_COUNT          32UL

#define BL_PARAM_MAGIC              0x5AA5C33CUL
#define BL_PARAM_TAIL_MAGIC         0xA5A5C3C3UL
#define BL_PARAM_VERSION            0x00010001UL

/*
 * APP 写完 APP2 后置 PENDING，后续状态由 BootLoader 校验和搬运后更新。
 */
#define BL_UPDATE_FLAG_IDLE         0x00000000UL
#define BL_UPDATE_FLAG_PENDING      0xAA55AA55UL
#define BL_UPDATE_FLAG_FAILED       0xDEAD0001UL
#define BL_UPDATE_FLAG_FAST_UPGRADE 0x55AA55AAUL

#define BL_ERR_NONE                 0UL
#define BL_ERR_PARAM_INVALID        1UL
#define BL_ERR_APP2_INVALID         2UL
#define BL_ERR_COPY_FAILED          3UL
#define BL_ERR_APP1_INVALID         4UL

#define BL_LOG_MAGIC                0xB100B100UL
#define BL_LOG_EVENT_PARAM_RECOVER  1UL
#define BL_LOG_EVENT_UPDATE_OK      2UL
#define BL_LOG_EVENT_UPDATE_FAIL    3UL
#define BL_LOG_EVENT_JUMP_FAIL      4UL

/* APP2 拷贝到 APP1 时用的小缓冲。 */
#define BL_COPY_CHUNK_SIZE          256UL

/* APP 和 BootLoader 共用的升级状态，CRC 不包含 param_crc32。 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t update_flag;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app1_addr;
    uint32_t app2_addr;
    uint32_t update_counter;
    uint32_t fail_counter;
    uint32_t last_error;
    uint32_t log_write_index;
    uint32_t reserved[51];
    uint32_t param_crc32;
    uint32_t tail_magic;
} bl_param_t;

/* 参数页环形区里的固定日志记录。 */
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t event_id;
    uint32_t result;
    uint32_t value0;
    uint32_t value1;
    uint32_t value2;
    uint32_t crc32;
} bl_log_entry_t;

bool bl_commit_param(bl_param_t *param);

#endif /* COMMON_BL_PARAM_H */
