/* 板级初始化和主循环*/
#include "gd32f4xx.h"
#include "systick.h"
#include "bl_bsp.h"
#include "bl_core.h"

int main(void)
{
    SCB->VTOR = 0x08000000U;

    systick_config();

    int upgrade = bl_check_upgrade_flag();
    bl_uart_init();          
    if (!upgrade) {
        bl_oled_show_bootloader();
    }

    bootloader_run(upgrade); 

    while (1) { }
}
