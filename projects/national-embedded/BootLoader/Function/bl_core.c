/* BootLoader OTA flow. */
#include "gd32f4xx.h"
#include <string.h>
#include "systick.h"
#include "protocol.h"
#include "ota_def.h"
#include "bl_bsp.h"
#include "bl_core.h"

#define FW_BUF_MAX   (48u * 1024u)
static uint8_t  s_fw[FW_BUF_MAX];
static uint32_t s_fw_len = 0;
static char     s_tx[PROTO_MAX_ASCII];

static int fmc_ready(void)
{
    uint32_t t = 0x3FFFFFu;
    while ((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && t) t--;
    return t > 0;
}

static void fmc_clear(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

static int flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t p = addr & ~(FLASH_PAGE_SIZE - 1u);
    uint32_t e = (addr + size - 1u) & ~(FLASH_PAGE_SIZE - 1u);

    fmc_unlock();
    fmc_clear();
    while (p <= e)
    {
        fmc_page_erase(p);
        if (!fmc_ready())
        {
            fmc_lock();
            return 0;
        }
        fmc_clear();
        p += FLASH_PAGE_SIZE;
    }
    fmc_lock();
    return 1;
}

static int flash_write(uint32_t addr, const uint8_t *d, uint32_t size)
{
    fmc_unlock();
    fmc_clear();
    while (((addr & 3u) == 0u) && (size >= 4u))
    {
        uint32_t w;
        memcpy(&w, d, 4);
        fmc_word_program(addr, w);
        if (!fmc_ready())
        {
            fmc_lock();
            return 0;
        }
        addr += 4u;
        d += 4u;
        size -= 4u;
    }
    while (size > 0u)
    {
        fmc_byte_program(addr, *d);
        if (!fmc_ready())
        {
            fmc_lock();
            return 0;
        }
        addr++;
        d++;
        size--;
    }
    fmc_clear();
    fmc_lock();
    return 1;
}

typedef void (*app_entry_t)(void);

static int app_vector_valid(uint32_t base)
{
    uint32_t sp = *(volatile uint32_t *)base;
    uint32_t rh = *(volatile uint32_t *)(base + 4u);

    if ((sp & 0xFF000000u) != 0x20000000u) return 0;
    if ((rh & 0xFF000000u) != 0x08000000u) return 0;
    return 1;
}

static void jump_to_app(uint32_t base)
{
    app_entry_t entry;
    uint32_t i;

    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    for (i = 0; i < 8u; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }
    __DSB();
    __ISB();
    SCB->VTOR = base;
    __set_MSP(*(volatile uint32_t *)base);
    entry = (app_entry_t)(*(volatile uint32_t *)(base + 4u));
    __enable_irq();
    entry();
}
/* Build one ASCII hex frame. */
static void send_frame(uint8_t type, uint16_t cmd, const uint8_t *pl, uint8_t len)
{
    uint32_t n = proto_build_ascii(bl_device_id, type, cmd, pl, len, s_tx, sizeof(s_tx));
    if (n) bl_uart_send((const uint8_t *)s_tx, n);
}
static void send_ok(uint16_t cmd)      { uint8_t f = ACK_OK; send_frame(FRAME_ACK, cmd, &f, 1); }
static void send_err_cmd(uint16_t cmd) { send_frame(FRAME_ERROR, cmd, NULL, 0); }

static int recv_firmware(void)
{
    uint32_t idle = 0, total = 0;
    s_fw_len = 0;
    bl_rx_flag = 0;

    while (total < 30000u)
    {
        if (bl_rx_flag)
        {
            uint32_t n = bl_rx_len;
            bl_rx_flag = 0;
            if (s_fw_len + n <= FW_BUF_MAX) { memcpy(s_fw + s_fw_len, bl_rx_buf, n); s_fw_len += n; }
            idle = 0;
        }
        else
        {
            delay_1ms(1);
            total++;
            if (s_fw_len > 0u) { idle++; if (idle > 800u) break; }
        }
    }

    return (s_fw_len >= 4u &&
            s_fw[0] == FW_MAGIC0 && s_fw[1] == FW_MAGIC1 &&
            s_fw[2] == FW_MAGIC2 && s_fw[3] == FW_MAGIC3) ? 1 : 0;
}

static int program_app(void)
{
    uint32_t img = s_fw_len - 4u;
    if (img == 0u || img > FLASH_APP_SIZE) return 0;
    if (!flash_erase(FLASH_APP_ADDR, img)) return 0;
    if (!flash_write(FLASH_APP_ADDR, s_fw + 4u, img)) return 0;
    return 1;
}

static void upgrade_mode(void)
{
    proto_frame_t fr;
    uint32_t idle = 0;
    int committed = 0;     
    int fw_ready  = 0;     

    bl_rx_flag = 0;
    delay_1ms(100);
    bl_oled_show_bootloader();
    idle = 60;

    while (1)
    {
        if (bl_rx_flag)
        {
            bl_rx_flag = 0;
            idle = 0;
            if (proto_parse_ascii((const char *)bl_rx_buf, bl_rx_len, &fr, 0) == PROTO_OK &&
                (fr.dev_id == bl_device_id || fr.dev_id == ID_BROADCAST) &&
                fr.type == FRAME_CMD)
            {
                if (fr.cmd == CMD_UPGRADE_PREPARE)         
                {
                    committed = 1;
                    fw_ready = recv_firmware();
                    if (fw_ready) send_ok(CMD_UPGRADE_PREPARE);
                    else          send_err_cmd(CMD_UPGRADE_PREPARE);
                    idle = 0;
                }
                else if (fr.cmd == CMD_UPGRADE_EXEC)       
                {
                    if (fw_ready)
                    {
                        send_ok(CMD_UPGRADE_EXEC);         
                        if (program_app())
                        {
                            delay_1ms(50);
                            NVIC_SystemReset();
                        }
                    }
                    else
                    {
                        send_err_cmd(CMD_UPGRADE_EXEC);
                    }
                }
            }
        }
        else
        {
            delay_1ms(1);
            idle++;
            if (!committed && idle > 10000u) return;   
            if (committed && idle > 45000u)  return;   
        }
    }
}

void bootloader_run(int upgrade)
{
    if (upgrade)
    {
        upgrade_mode();                        
    }
    else
    {
        delay_1ms(5000);                       
    }

    if (app_vector_valid(FLASH_APP_ADDR))
    {
        jump_to_app(FLASH_APP_ADDR);
    }
    while (1) { }
}

void bl_log_dump_uart(void) { }   
