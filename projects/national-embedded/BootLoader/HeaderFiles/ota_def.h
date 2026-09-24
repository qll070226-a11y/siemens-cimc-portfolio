/* OTA Flash 分区常量。 */
#ifndef OTA_DEF_H
#define OTA_DEF_H

#include <stdint.h>

/* OTA Flash 分区常量。 */
#define FLASH_BOOT_ADDR      0x08000000u   
#define FLASH_BOOT_SIZE      0x00010000u
#define FLASH_PARAM_ADDR     0x08010000u   
#define FLASH_APP_ADDR       0x08011000u   
#define FLASH_APP_SIZE       0x00020000u
#define FLASH_BACKUP_ADDR    0x08031000u   
#define FLASH_STAGING_ADDR   0x08051000u   
#define FLASH_STAGING_SIZE   0x00020000u
#define FLASH_PAGE_SIZE      0x00001000u   

/* OTA Flash 分区常量。 */
#define FW_MAGIC0            0x5Au
#define FW_MAGIC1            0xA5u
#define FW_MAGIC2            0xC3u
#define FW_MAGIC3            0x3Cu

/* OTA Flash 分区常量。 */
#define OTA_FLAG_MAGIC       0xAA55AA55u

#endif /* OTA_DEF_H */
