/*!
    \file    gd30ad3344.c
    \brief   gd30ad3344 driver
    
    \version 2024-10-08, V1.0.0, firmware for GD30AD3344
*/

#include "gd30ad3344.h"

extern uint8_t spi3_send_array[ARRAYSIZE];    
extern uint8_t spi3_receive_array[ARRAYSIZE]; 


uint8_t spi_gd30ad3344_send_byte_dma(uint8_t byte)
{
    
    spi3_send_array[0] = byte;
    
    
    dma_single_data_parameter_struct dma_init_struct;
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 1;  
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));

    /* DMA completion only means the last byte reached SPI_DATA. Wait until
       the final clock edge is sent before disabling DMA and releasing CS. */
    while(SET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_TRANS));
    
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    
    return spi3_receive_array[0];
}


uint16_t spi_gd30ad3344_send_halfword_dma(uint16_t half_word)
{
    GD30_CS_LOW();
    uint16_t rx_data;
    
    
    spi3_send_array[0] = (uint8_t)(half_word >> 8);
    spi3_send_array[1] = (uint8_t)half_word;
    
    
    dma_single_data_parameter_struct dma_init_struct;
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 2;  
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));

    /* DMA completion only means the last byte reached SPI_DATA. Wait until
       the final clock edge is sent before disabling DMA and releasing CS. */
    while(SET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_TRANS));
    
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    
    rx_data = (uint16_t)(spi3_receive_array[0] << 8);
    rx_data |= spi3_receive_array[1];
    GD30_CS_HIGH();
    return rx_data;
}


void spi_gd30ad3344_transmit_receive_dma(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size)
{
    
    if (size > ARRAYSIZE) {
        size = ARRAYSIZE;
    }
    
    
    for (uint16_t i = 0; i < size; i++) {
        spi3_send_array[i] = tx_buffer[i];
    }
    
    
    dma_single_data_parameter_struct dma_init_struct;
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = size;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));

    /* DMA completion only means the last byte reached SPI_DATA. Wait until
       the final clock edge is sent before disabling DMA and releasing CS. */
    while(SET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_TRANS));
    
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    
    for (uint16_t i = 0; i < size; i++) {
        rx_buffer[i] = spi3_receive_array[i];
    }
}


void spi_gd30ad3344_wait_for_dma_end(void)
{
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));
    
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
}


GD30AD3344 GD30AD3344_InitStruct;

#define GD30AD3344_CONVERSION_TIMEOUT_MS 10U

static float s_gd30ad3344_last_result[8];

static uint8_t gd30ad3344_wait_data_ready(uint32_t timeout_ms)
{
    uint32_t start_ms = (uint32_t)get_system_ms();

    GD30_CS_LOW();

    /* DOUT/DRDY can still be low from the previous conversion. Require the
       new single-shot conversion to enter busy/high before accepting ready. */
    while (RESET == gpio_input_bit_get(GD30_SPI_PORT, GD30_SPI_MISO)) {
        if ((uint32_t)((uint32_t)get_system_ms() - start_ms) >= timeout_ms) {
            GD30_CS_HIGH();
            return 0U;
        }
    }

    /* This high-to-low transition belongs to the requested MUX channel. */
    while (SET == gpio_input_bit_get(GD30_SPI_PORT, GD30_SPI_MISO)) {
        if ((uint32_t)((uint32_t)get_system_ms() - start_ms) >= timeout_ms) {
            GD30_CS_HIGH();
            return 0U;
        }
    }

    return 1U;
}

void GD30AD3344_Init(void)
{
    GD30AD3344_InitStruct.SS         = GD30AD3344_OS_DISABLE;
    GD30AD3344_InitStruct.MUX        = GD30AD3344_MUX_AIN0_GND;
                                                //AIN0~AIN1 AIN0~AIN3 AIN1~AIN3 AIN2~AIN3 AIN0~GND  AIN1~GND  AIN2~GND  AIN3~GND 
    GD30AD3344_InitStruct.PGA        = GD30AD3344_PGA_2V048;
                                                
    GD30AD3344_InitStruct.MODE       = GD30AD3344_MODE_SINGLE_SHOT;
    GD30AD3344_InitStruct.DR         = GD30AD3344_DR_250SPS;
                                                //  6.25SPS     12.5SPS   25SPS     50SPS     100SPS    250SPS    500SPS    1000SPS
    GD30AD3344_InitStruct.RESERVED_1 = GD30AD3344_RESERVED_0;
    GD30AD3344_InitStruct.PULL_UP_EN = GD30AD3344_PULL_UP_DISABLE;
    GD30AD3344_InitStruct.NOP        = GD30AD3344_NOP_VALID_UPDATE;
    GD30AD3344_InitStruct.RESERVED   = GD30AD3344_RESERVED_1;
    
    spi_enable(GD30_SPI);
    spi_gd30ad3344_send_halfword_dma(GD30AD3344_InitStruct_Value);
    my_printf(DEBUG_USART, "0x%4X", GD30AD3344_InitStruct_Value);
}

float ADS118_PGA_SET(GD30AD3344_PGA_TypeDef PGA)
{
    switch(PGA) {
    case GD30AD3344_PGA_6V144:
        return 6.144f;
    case GD30AD3344_PGA_4V096:
        return 4.096f;
    case GD30AD3344_PGA_2V048:
        return 2.048f;
    case GD30AD3344_PGA_1V024:
        return 1.024f;
    case GD30AD3344_PGA_0V512:
        return 0.512f;
    case GD30AD3344_PGA_0V256:
        return 0.256f;
    case GD30AD3344_PGA_0V064:
        return 0.064f;
    default:
        return 2.048f;
    }
}

float GD30AD3344_AD_Read(GD30AD3344_Channel_TypeDef CH, GD30AD3344_PGA_TypeDef Ref)
{
    uint16_t raw_data;
    uint8_t channel_index = (uint8_t)CH;
    float result = 0.0;

    /* Start one conversion for the selected input. The first SPI response is
       the previous conversion result, so it must not be returned to caller. */
    GD30AD3344_InitStruct.SS = GD30AD3344_OS_SINGLE_CONVERT;
    GD30AD3344_InitStruct.MUX = CH;
    GD30AD3344_InitStruct.PGA = Ref;
    GD30AD3344_InitStruct.MODE = GD30AD3344_MODE_SINGLE_SHOT;
    GD30AD3344_InitStruct.DR = GD30AD3344_DR_250SPS;
    GD30AD3344_InitStruct.NOP = GD30AD3344_NOP_VALID_UPDATE;

    (void)spi_gd30ad3344_send_halfword_dma(GD30AD3344_InitStruct_Value);

    /* A 250-SPS conversion takes 4 ms. Allow a full extra conversion period,
       then discard the first result after a MUX change. Writing the same
       single-shot configuration here also starts a second conversion on the
       already-settled channel. */
    delay_ms(8U);
    (void)spi_gd30ad3344_send_halfword_dma(GD30AD3344_InitStruct_Value);
    delay_ms(8U);

    /* Clock out the second completed conversion without changing Config. */
    raw_data = spi_gd30ad3344_send_halfword_dma(0x0000U);
    
    result = (float)((int16_t)raw_data) * ADS118_PGA_SET(Ref) / 32768.0f;
    s_gd30ad3344_last_result[channel_index] = result;
    return (float)result;
}
