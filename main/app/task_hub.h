/*
 * FreeRTOS tasks handler
 */

#ifndef __TASK_HUB_H__
#define __TASK_HUB_H__


#include <stdint.h>
#include <stdbool.h>


void task_hub_tasks_create();
void task_hub_I2S_buf_init();
void task_hub_I2S_create();
void task_hub_I2S_del();
void task_hub_ringbuf_send(const uint8_t *data, size_t size);
void task_hub_set_volume(uint8_t vol);


#endif /* __TASK_HUB_H__ */
