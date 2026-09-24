#ifndef COMMON_BL_PARTITION_H
#define COMMON_BL_PARTITION_H

#include <stdint.h>

/*
 * APP 和 BootLoader 共用的 Flash 分区。
 * APP1 是运行区，APP2 是升级暂存区；复位后由 BootLoader 判断是否搬运。
 */
#define BL_FLASH_BASE_ADDR          0x08000000UL
#define BL_FLASH_TOTAL_SIZE         0x00080000UL
#define BL_FLASH_END_ADDR           (BL_FLASH_BASE_ADDR + BL_FLASH_TOTAL_SIZE - 1UL)
#define BL_FLASH_PAGE_SIZE          0x00001000UL

/* BootLoader 镜像区，链接脚本要落在这里。 */
#define BL_BOOT_START_ADDR          0x08000000UL
#define BL_BOOT_SIZE                0x0000C000UL
#define BL_BOOT_END_ADDR            (BL_BOOT_START_ADDR + BL_BOOT_SIZE - 1UL)

/* 参数页：主参数、备份参数和少量日志。 */
#define BL_PARAM_START_ADDR         0x0800C000UL
#define BL_PARAM_SIZE               0x00001000UL

/* APP 运行区，校验通过后跳转。 */
#define BL_APP1_START_ADDR          0x0800D000UL
#define BL_APP1_SIZE                0x00038000UL
#define BL_APP1_END_ADDR            (BL_APP1_START_ADDR + BL_APP1_SIZE - 1UL)

/* OTA 暂存区，APP 写入新固件，BootLoader 搬到 APP1。 */
#define BL_APP2_START_ADDR          0x08045000UL
#define BL_APP2_SIZE                0x00038000UL
#define BL_APP2_END_ADDR            (BL_APP2_START_ADDR + BL_APP2_SIZE - 1UL)

/* 用户数据区，升级搬运时不处理。 */
#define BL_DATA_START_ADDR          0x0807D000UL
#define BL_DATA_SIZE                0x00003000UL
#define BL_DATA_END_ADDR            (BL_DATA_START_ADDR + BL_DATA_SIZE - 1UL)

#endif /* COMMON_BL_PARTITION_H */
