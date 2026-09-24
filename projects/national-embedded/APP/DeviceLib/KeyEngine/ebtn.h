/* Button event engine. */
#ifndef _EBTN_H
#define _EBTN_H

#include <stdint.h>
#include <string.h>

#include "bit_array.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// #define EBTN_CONFIG_TIMER_16


#ifdef EBTN_CONFIG_TIMER_16
typedef uint16_t ebtn_time_t;
typedef int16_t ebtn_time_sign_t;
#else
typedef uint32_t ebtn_time_t;
typedef int32_t ebtn_time_sign_t;
#endif


struct ebtn_btn;
struct ebtn;

#define EBTN_MAX_KEYNUM (64)  

#define EBTN_DEFAULT_COMBO_SUPPRESS_THRESHOLD 500  


typedef enum
{
    EBTN_EVT_ONPRESS = 0x00,  
    EBTN_EVT_ONRELEASE,       
    EBTN_EVT_ONCLICK,         
    EBTN_EVT_KEEPALIVE,       
} ebtn_evt_t;


#define EBTN_EVT_MASK_ONPRESS   (1 << EBTN_EVT_ONPRESS)    
#define EBTN_EVT_MASK_ONRELEASE (1 << EBTN_EVT_ONRELEASE)  
#define EBTN_EVT_MASK_ONCLICK   (1 << EBTN_EVT_ONCLICK)    
#define EBTN_EVT_MASK_KEEPALIVE (1 << EBTN_EVT_KEEPALIVE)  

#define EBTN_EVT_MASK_ALL (EBTN_EVT_MASK_ONPRESS | EBTN_EVT_MASK_ONRELEASE | EBTN_EVT_MASK_ONCLICK | EBTN_EVT_MASK_KEEPALIVE)  


#define EBTN_CFG_COMBO_PRIORITY (1 << 0)  


static inline ebtn_time_sign_t ebtn_timer_sub(ebtn_time_t time1, ebtn_time_t time2)
{
    return time1 - time2;
}


// #define ebtn_timer_sub(time1, time2) (time1 - time2)


typedef void (*ebtn_evt_fn)(struct ebtn_btn *btn, ebtn_evt_t evt);


typedef uint8_t (*ebtn_get_state_fn)(struct ebtn_btn *btn);


typedef struct ebtn_btn_param
{
    
    uint16_t time_debounce;  

    
    uint16_t time_debounce_release;  

    
    uint16_t time_click_pressed_min;  

    
    uint16_t time_click_pressed_max;  

    
    uint16_t time_click_multi_max;  

    
    uint16_t time_keepalive_period;  

    
    uint16_t max_consecutive;  
} ebtn_btn_param_t;


#define EBTN_PARAMS_INIT(_time_debounce, _time_debounce_release, _time_click_pressed_min, _time_click_pressed_max, _time_click_multi_max,                      \
                         _time_keepalive_period, _max_consecutive)                                                                                             \
    {                                                                                                                                                          \
        .time_debounce = _time_debounce, .time_debounce_release = _time_debounce_release, .time_click_pressed_min = _time_click_pressed_min,                   \
        .time_click_pressed_max = _time_click_pressed_max, .time_click_multi_max = _time_click_multi_max, .time_keepalive_period = _time_keepalive_period,     \
        .max_consecutive = _max_consecutive                                                                                                                    \
    }


#define EBTN_BUTTON_INIT_RAW(_key_id, _param, _mask)                                                                                                           \
    {                                                                                                                                                          \
        .key_id = _key_id, .param = _param, .event_mask = _mask,                                                                                               \
    }


#define EBTN_BUTTON_INIT(_key_id, _param) EBTN_BUTTON_INIT_RAW(_key_id, _param, EBTN_EVT_MASK_ALL)


#define EBTN_BUTTON_DYN_INIT(_key_id, _param)                                                                                                                  \
    {                                                                                                                                                          \
        .next = NULL, .btn = EBTN_BUTTON_INIT(_key_id, _param),                                                                                                \
    }


#define EBTN_BUTTON_COMBO_INIT_RAW(_key_id, _param, _mask)                                                                                                     \
    {                                                                                                                                                          \
        .comb_key = {0}, .btn = EBTN_BUTTON_INIT_RAW(_key_id, _param, _mask),                                                                                  \
    }


#define EBTN_BUTTON_COMBO_INIT(_key_id, _param)                                                                                                                \
    {                                                                                                                                                          \
        .comb_key = {0}, .btn = EBTN_BUTTON_INIT(_key_id, _param),                                                                                             \
    }


#define EBTN_BUTTON_COMBO_DYN_INIT(_key_id, _param)                                                                                                            \
    {                                                                                                                                                          \
        .next = NULL, .btn = EBTN_BUTTON_COMBO_INIT(_key_id, _param),                                                                                          \
    }


#define EBTN_ARRAY_SIZE(_arr) sizeof(_arr) / sizeof((_arr)[0])


typedef struct ebtn_btn
{
    uint16_t key_id;     
    uint8_t flags;       
    uint8_t event_mask;  

    ebtn_time_t time_change;        
    ebtn_time_t time_state_change;  

    ebtn_time_t keepalive_last_time;  
    ebtn_time_t click_last_time;      

    uint16_t keepalive_cnt;  
    uint16_t click_cnt;      

    const ebtn_btn_param_t *param;  
} ebtn_btn_t;


typedef struct ebtn_btn_combo
{
    BIT_ARRAY_DEFINE(comb_key, EBTN_MAX_KEYNUM);  

    ebtn_btn_t btn;  
} ebtn_btn_combo_t;


typedef struct ebtn_btn_dyn
{
    struct ebtn_btn_dyn *next;  

    ebtn_btn_t btn;  
} ebtn_btn_dyn_t;


typedef struct ebtn_btn_combo_dyn
{
    struct ebtn_btn_combo_dyn *next;  

    ebtn_btn_combo_t btn;  
} ebtn_btn_combo_dyn_t;


typedef struct ebtn
{
    ebtn_btn_t *btns;              
    uint16_t btns_cnt;             
    ebtn_btn_combo_t *btns_combo;  
    uint16_t btns_combo_cnt;       

    ebtn_btn_dyn_t *btn_dyn_head;              
    ebtn_btn_combo_dyn_t *btn_combo_dyn_head;  

    ebtn_evt_fn evt_fn;              
    ebtn_get_state_fn get_state_fn;  

    BIT_ARRAY_DEFINE(old_state, EBTN_MAX_KEYNUM);  
    BIT_ARRAY_DEFINE(combo_active, EBTN_MAX_KEYNUM);  
    
    uint8_t config;  

    
    uint16_t last_combo_event_id;        
    ebtn_time_t last_combo_event_time;     
    ebtn_time_t combo_suppress_threshold;  
} ebtn_t;


void ebtn_process(ebtn_time_t mstime);


void ebtn_process_with_curr_state(bit_array_t *curr_state, ebtn_time_t mstime);


int ebtn_is_btn_active(const ebtn_btn_t *btn);


int ebtn_is_btn_in_process(const ebtn_btn_t *btn);


int ebtn_is_in_process(void);


int ebtn_init(ebtn_btn_t *btns, uint16_t btns_cnt, ebtn_btn_combo_t *btns_combo, uint16_t btns_combo_cnt, ebtn_get_state_fn get_state_fn, ebtn_evt_fn evt_fn);


int ebtn_register(ebtn_btn_dyn_t *button);


int ebtn_combo_register(ebtn_btn_combo_dyn_t *button);


int ebtn_get_total_btn_cnt(void);


int ebtn_get_btn_index_by_key_id(uint16_t key_id);


ebtn_btn_t *ebtn_get_btn_by_key_id(uint16_t key_id);


int ebtn_get_btn_index_by_btn(ebtn_btn_t *btn);


int ebtn_get_btn_index_by_btn_dyn(ebtn_btn_dyn_t *btn);


void ebtn_combo_btn_add_btn_by_idx(ebtn_btn_combo_t *btn, int idx);


void ebtn_combo_btn_remove_btn_by_idx(ebtn_btn_combo_t *btn, int idx);


void ebtn_combo_btn_add_btn(ebtn_btn_combo_t *btn, uint16_t key_id);


void ebtn_combo_btn_remove_btn(ebtn_btn_combo_t *btn, uint16_t key_id);


#define ebtn_keepalive_get_period(btn) ((btn)->time_keepalive_period)


#define ebtn_keepalive_get_count(btn) ((btn)->keepalive_cnt)


#define ebtn_keepalive_get_count_for_time(btn, ms_time) ((ms_time) / ebtn_keepalive_get_period(btn))


#define ebtn_click_get_count(btn) ((btn)->click_cnt)


void ebtn_set_config(uint8_t cfg_flags);


uint8_t ebtn_get_config(void);


void ebtn_set_combo_suppress_threshold(ebtn_time_t threshold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EBTN_H */
