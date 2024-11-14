/*
 * FreeRTOS tasks handler
 */

#ifndef __TASKS_H__
#define __TASKS_H__


#include <stdint.h>
#include <stdbool.h>


void tasks_create();
void tasks_I2S_buf_init();
void tasks_I2S_create();
void tasks_I2S_del();
void tasks_ringbuf_send(const uint8_t *data, size_t size);
void tasks_set_volume(uint8_t vol);


#endif /* __TASKS_H__ */
