/* BootLoader 板级驱动*/
#include "gd32f4xx.h"
#include "systick.h"
#include <string.h>
#include "ota_def.h"
#include "bl_bsp.h"

static uint8_t s_dma_rx[512];

uint8_t           bl_rx_buf[512];
volatile uint32_t bl_rx_len  = 0;
volatile uint8_t  bl_rx_flag = 0;
uint16_t          bl_device_id = 0x0001;
uint32_t          bl_baud = 19200u;


#define BL_OLED_I2C              I2C0
#define BL_OLED_ADDR_WRITE       0x78U
#define BL_OLED_PORT             GPIOB
#define BL_OLED_SCL_PIN          GPIO_PIN_8
#define BL_OLED_SDA_PIN          GPIO_PIN_9
#define BL_OLED_AF               GPIO_AF_4
#define BL_OLED_TIMEOUT          30000U
#define BL_TEAM_NUMBER           "2026931391"

typedef struct {
    char ch;
    uint8_t col[6];
} bl_oled_font_t;

static const bl_oled_font_t bl_oled_font[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00}},
    {'0', {0x00,0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x00,0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x00,0x21,0x41,0x45,0x4B,0x31}},
    {'6', {0x00,0x3C,0x4A,0x49,0x49,0x30}},
    {'9', {0x00,0x06,0x49,0x49,0x29,0x1E}},
    {'B', {0x00,0x7F,0x49,0x49,0x49,0x36}},
    {'a', {0x00,0x20,0x54,0x54,0x54,0x78}},
    {'d', {0x00,0x38,0x44,0x44,0x48,0x7F}},
    {'e', {0x00,0x38,0x54,0x54,0x54,0x18}},
    {'l', {0x00,0x00,0x41,0x7F,0x40,0x00}},
    {'o', {0x00,0x38,0x44,0x44,0x44,0x38}},
    {'r', {0x00,0x7C,0x08,0x04,0x04,0x08}},
    {'t', {0x00,0x04,0x3F,0x44,0x40,0x20}},
};

static int bl_oled_wait_flag(i2c_flag_enum flag)
{
    uint32_t timeout = BL_OLED_TIMEOUT;
    while (timeout--) {
        if (SET == i2c_flag_get(BL_OLED_I2C, flag)) return 1;
    }
    return 0;
}

static int bl_oled_wait_not_busy(void)
{
    uint32_t timeout = BL_OLED_TIMEOUT;
    while (timeout--) {
        if (RESET == i2c_flag_get(BL_OLED_I2C, I2C_FLAG_I2CBSY)) return 1;
    }
    return 0;
}

static int bl_oled_write_byte(uint8_t control, uint8_t value)
{
    if (!bl_oled_wait_not_busy()) return 0;

    i2c_start_on_bus(BL_OLED_I2C);
    if (!bl_oled_wait_flag(I2C_FLAG_SBSEND)) return 0;

    i2c_master_addressing(BL_OLED_I2C, BL_OLED_ADDR_WRITE, I2C_TRANSMITTER);
    if (!bl_oled_wait_flag(I2C_FLAG_ADDSEND)) return 0;
    i2c_flag_clear(BL_OLED_I2C, I2C_FLAG_ADDSEND);

    if (!bl_oled_wait_flag(I2C_FLAG_TBE)) return 0;
    i2c_data_transmit(BL_OLED_I2C, control);
    if (!bl_oled_wait_flag(I2C_FLAG_TBE)) return 0;
    i2c_data_transmit(BL_OLED_I2C, value);
    if (!bl_oled_wait_flag(I2C_FLAG_BTC)) return 0;

    i2c_stop_on_bus(BL_OLED_I2C);
    return 1;
}

static void bl_oled_cmd(uint8_t cmd)
{
    (void)bl_oled_write_byte(0x00U, cmd);
}

static void bl_oled_data(uint8_t data)
{
    (void)bl_oled_write_byte(0x40U, data);
}

static void bl_oled_set_pos(uint8_t x, uint8_t page)
{
    bl_oled_cmd((uint8_t)(0xB0U + page));
    bl_oled_cmd((uint8_t)(0x10U | ((x >> 4U) & 0x0FU)));
    bl_oled_cmd((uint8_t)(x & 0x0FU));
}

static const uint8_t *bl_oled_find_font(char ch)
{
    uint32_t i;
    for (i = 0; i < (sizeof(bl_oled_font) / sizeof(bl_oled_font[0])); i++) {
        if (bl_oled_font[i].ch == ch) return bl_oled_font[i].col;
    }
    return bl_oled_font[0].col;
}

static void bl_oled_show_char(uint8_t x, uint8_t page, char ch)
{
    const uint8_t *col = bl_oled_find_font(ch);
    uint32_t i;
    bl_oled_set_pos(x, page);
    for (i = 0; i < 6U; i++) bl_oled_data(col[i]);
}

static void bl_oled_show_str(uint8_t x, uint8_t page, const char *s)
{
    uint8_t j = 0U;
    while (s[j] != '\0') {
        bl_oled_show_char(x, page, s[j]);
        x = (uint8_t)(x + 8U);
        if (x > 120U) {
            x = 0U;
            page = (uint8_t)(page + 2U);
        }
        j++;
    }
}

static void bl_oled_line(uint8_t page, const char *s)
{
    char buf[22];
    uint8_t i = 0U;
    while (s[i] && i < 20U) { buf[i] = s[i]; i++; }
    while (i < 20U) { buf[i++] = ' '; }
    buf[i] = '\0';
    bl_oled_show_str(0U, page, buf);
}
static void bl_oled_clear(void)
{
    uint8_t page, x;
    for (page = 0; page < 4U; page++) {
        bl_oled_set_pos(0U, page);
        for (x = 0; x < 128U; x++) bl_oled_data(0x00U);
    }
}

void bl_oled_show_bootloader(void)
{
    static const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x1F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0xA1, 0xC8, 0xDA, 0x00, 0x81, 0x80,
        0xD9, 0x1F, 0xDB, 0x40, 0xA4, 0xAF
    };
    uint32_t i;

    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    gpio_af_set(BL_OLED_PORT, BL_OLED_AF, BL_OLED_SCL_PIN | BL_OLED_SDA_PIN);
    gpio_mode_set(BL_OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BL_OLED_SCL_PIN | BL_OLED_SDA_PIN);
    gpio_output_options_set(BL_OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, BL_OLED_SCL_PIN | BL_OLED_SDA_PIN);

    i2c_deinit(BL_OLED_I2C);
    i2c_clock_config(BL_OLED_I2C, 400000U, I2C_DTCY_2);
    i2c_mode_addr_config(BL_OLED_I2C, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x72U);
    i2c_enable(BL_OLED_I2C);
    i2c_ack_config(BL_OLED_I2C, I2C_ACK_ENABLE);
    delay_1ms(10);

    for (i = 0; i < (sizeof(init_cmds) / sizeof(init_cmds[0])); i++) {
        bl_oled_cmd(init_cmds[i]);
    }

    bl_oled_clear();
    bl_oled_line(0U, BL_TEAM_NUMBER);
    bl_oled_line(2U, "Bootloader");
}

static void rs485_dir_tx(int tx)
{
    if (tx) GPIO_BOP(GPIOE) = GPIO_PIN_8;   
    else    GPIO_BC(GPIOE)  = GPIO_PIN_8;   
}

void bl_uart_init(void)
{
    dma_single_data_parameter_struct di;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_USART1);

    /* USART1：TX=PD5，RX=PD6*/
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_6);

    /* BootLoader 板级驱动*/
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_8);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    rs485_dir_tx(0);

    /* DMA0 CH5 SUB4 接 USART1_RX*/
    dma_deinit(DMA0, DMA_CH5);
    di.direction            = DMA_PERIPH_TO_MEMORY;
    di.memory0_addr         = (uint32_t)s_dma_rx;
    di.memory_inc           = DMA_MEMORY_INCREASE_ENABLE;
    di.number               = sizeof(s_dma_rx);
    di.periph_addr          = (uint32_t)&USART_DATA(USART1);
    di.periph_inc           = DMA_PERIPH_INCREASE_DISABLE;
    di.periph_memory_width  = DMA_PERIPH_WIDTH_8BIT;
    di.priority             = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH5, &di);
    dma_circulation_disable(DMA0, DMA_CH5);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);
    dma_channel_enable(DMA0, DMA_CH5);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, bl_baud);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_enable(USART1);

    nvic_irq_enable(USART1_IRQn, 1, 0);
    usart_interrupt_enable(USART1, USART_INT_IDLE);
}

void USART1_IRQHandler(void)
{
    uint32_t n;
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE))
    {
        usart_data_receive(USART1);             
        dma_channel_disable(DMA0, DMA_CH5);
        n = sizeof(s_dma_rx) - dma_transfer_number_get(DMA0, DMA_CH5);
        if (n > 0 && n <= sizeof(s_dma_rx))
        {
            uint32_t cp = n;
            if (cp >= sizeof(bl_rx_buf)) cp = sizeof(bl_rx_buf) - 1;
            memcpy(bl_rx_buf, s_dma_rx, cp);
            bl_rx_len  = cp;
            bl_rx_flag = 1;
        }
        memset(s_dma_rx, 0, sizeof(s_dma_rx));
        dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF);
        dma_transfer_number_config(DMA0, DMA_CH5, sizeof(s_dma_rx));
        dma_channel_enable(DMA0, DMA_CH5);
    }
}

void bl_uart_send(const uint8_t *data, uint32_t len)
{
    uint32_t i;
    rs485_dir_tx(1);
    for (i = 0; i < len; i++)
    {
        usart_data_transmit(USART1, data[i]);
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
    }
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    rs485_dir_tx(0);
}

void bl_uart_send_str(const char *s)
{
    bl_uart_send((const uint8_t *)s, (uint32_t)strlen(s));
}

int bl_check_upgrade_flag(void)
{
    /* BootLoader 板级驱动*/
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    if (RTC_BKP1 == OTA_FLAG_MAGIC)
    {
        uint16_t id = (uint16_t)RTC_BKP2;
        uint8_t  bc = (uint8_t)RTC_BKP3;
        if (id >= 0x0001u && id <= 0xFFFEu) bl_device_id = id;
        switch (bc) {                 
            case 0x11: bl_baud = 4800u;   break;
            case 0x12: bl_baud = 9600u;   break;
            case 0x13: bl_baud = 19200u;  break;
            case 0x14: bl_baud = 115200u; break;
            default:  break;
        }
        RTC_BKP1 = 0;        
        return 1;
    }
    return 0;
}
