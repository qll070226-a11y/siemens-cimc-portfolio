#include "mcu_cmic_gd32f470vet6.h"

/* 任务表数量。 */
uint8_t task_num;

typedef struct {
    void (*task_func)(void);
    uint32_t rate_ms;
    uint32_t last_run;
} task_t;

/* 固定任务表：函数、周期和上次运行时间。 */
static task_t scheduler_task[] =
{
     {led_task,  1,    0}
    ,{adc_task,  100,  0}
    ,{oled_task, 10,   0}
    ,{btn_task,  5,    0}
    ,{uart_task, 5,    0}
    ,{rtc_task,  500,  0}

};

/* 初始化任务数量。 */
void scheduler_init(void)
{
    task_num = sizeof(scheduler_task) / sizeof(task_t);
}

/* 按周期运行到时任务。 */
void scheduler_run(void)
{
    for (uint8_t i = 0; i < task_num; i++)
    {
        uint32_t now_time = get_system_ms();
        if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run)
        {
            scheduler_task[i].last_run = now_time;
            scheduler_task[i].task_func();
        }
    }
}


