/*
 * FreeRTOS tasks handler
 */

#ifndef __TASK_HUB_H__
#define __TASK_HUB_H__


#include <stdint.h>
#include <stdbool.h>


/* handler for the dispatched work */
typedef void (* bt_app_cb_t) (uint16_t event, void *param);

/* message to be sent */
typedef struct {
    uint16_t       event;    /* message event id */
    bt_app_cb_t    cb;       /* context switch callback */
    void           *param;   /* parameter area needs to be last */
} bt_app_msg_t;


/*
 * work dispatcher for the application task
 *
 * p_cback:       callback function
 * event:         event id
 * p_params:      callback paramters
 * param_len:     parameter length in byte
 * p_copy_cback:  parameter deep-copy function
 *
 * return:
 *  true if work dispatch successfully, false otherwise
 */
bool task_hub_bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len);

void task_hub_tasks_create();
void task_hub_bt_app_process();
void task_hub_I2S_buf_init();
void task_hub_I2S_create();
void task_hub_I2S_del();
void task_hub_ringbuf_send(const uint8_t *data, size_t size);
void task_hub_set_volume(uint8_t vol);


#endif /* __TASK_HUB_H__ */
