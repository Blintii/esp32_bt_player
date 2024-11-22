/*
 * FreeRTOS tasks handler
 */

#ifndef __TASKS_H__
#define __TASKS_H__


#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"


#define TASKS_THROTTLE_MIN_TIME pdMS_TO_TICKS(100)


typedef enum {
    TASKS_INST_UNKNOWN,
    TASKS_INST_THROTTLER,
    TASKS_INST_AUDIO_PLAYER,
    TASKS_INST_AUDIO_DSP,
    TASKS_INST_LIGHT,
    TASKS_INST_HOTSPOT,
    TASKS_INST_MAX
} tasks_instance;

typedef enum {
    TASKS_SIG_UNKNOWN,
    TASKS_SIG_AUDIO_STREAM_STARTED,
    TASKS_SIG_AUDIO_STREAM_SUSPEND,
    TASKS_SIG_AUDIO_DATA_SUFFICIENT,
    TASKS_SIG_AUDIO_VOLUME,
    TASKS_SIG_MAX
} tasks_signal_type;

typedef union {
    union {
        uint8_t volume;
    } audio_volume;
} tasks_signal_arg;

typedef struct {
    tasks_instance aim_task;
    tasks_signal_type type;
    tasks_signal_arg arg;
} tasks_signal;

typedef struct {
    tasks_signal waiting_signal;
    TickType_t tick_left;
    bool live;
    bool started;
    bool need_send;
    bool need_acknowledge;
} tasks_signal_throttled;


void tasks_create();
void tasks_message(tasks_signal signal);
void tasks_audio_data(const uint8_t *data, size_t size);


#endif /* __TASKS_H__ */
