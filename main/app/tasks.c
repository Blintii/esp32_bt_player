
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#include "app_config.h"
#include "app_tools.h"
#include "tasks.h"
#include "ach.h"
#include "bt_profiles.h"
#include "lights.h"


#define RINGBUF_SIZE (40 * 1024)
#define RINGBUF_TRIGGER_LEVEL (16 * 1024)


typedef enum {
    AUDIO_STATE_INIT,
    AUDIO_STATE_STOP, // audio input stream off, play out off
    AUDIO_STATE_READY, // audio stream started, waiting first audio packet
    AUDIO_STATE_PRELOAD, // audio stream coming, buffered data exist, but not enough to play out
    AUDIO_STATE_PLAY, // audio stream coming, playing out buffered data
    AUDIO_STATE_DROP, // audio stream too fast, can't playing out all buffered data
    AUDIO_STATE_FLUSH // audio stream off, but buffer need play out remaining data
} audio_state_t;


static void tasks_throttler();
static void tasks_handle_throttled_signal(tasks_signal_throttled *signal);
static void tasks_send_throttled_signal(tasks_signal signal, TickType_t throttle_min_time);
static void tasks_audio_player();
static void tasks_audio_state(audio_state_t new_state);
static void tasks_signal_send(tasks_signal signal);
static void tasks_lights();
// static void throttled_signal_dumb(tasks_signal_throttled *signal);


static const char *TAG = LOG_COLOR("91") "TASK" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("91") "TASK" LOG_COLOR_E;
static SemaphoreHandle_t throttler_semaphore = NULL;
static SemaphoreHandle_t audio_semaphore = NULL;
static QueueHandle_t mails_audio_player = NULL;
static RingbufHandle_t audio_stream_ringbuf = NULL;
static audio_state_t audio_state = AUDIO_STATE_INIT;
static tasks_signal_throttled throttled_signals[TASKS_INST_MAX][TASKS_SIG_MAX] = {0};
static size_t dropped_bytes = 0;


void tasks_create()
{
    ESP_LOGI(TAG, "init variables...");
    throttler_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(throttler_semaphore);
    xSemaphoreGive(throttler_semaphore);
    audio_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(audio_semaphore);
    xSemaphoreGive(audio_semaphore);
    mails_audio_player = xQueueCreate(15, sizeof(tasks_signal));
    ERR_IF_NULL_RESET(mails_audio_player);
    audio_stream_ringbuf = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    ERR_IF_NULL_RESET(audio_stream_ringbuf);

    ESP_LOGI(TAG, "init variables OK");
    xTaskCreatePinnedToCore(tasks_throttler, "Throttler", 4096, NULL, 12, NULL, 1);
    xTaskCreatePinnedToCore(tasks_audio_player, "Audio Player", 4096, NULL, 12, NULL, 1);
    xTaskCreatePinnedToCore(tasks_lights, "Lights", 4096, NULL, 12, NULL, 1);
    ESP_LOGI(TAG, "created tasks OK");
}

void tasks_message(tasks_signal signal)
{
    switch(signal.type)
    {
        case TASKS_SIG_UNKNOWN: ESP_LOGW(TAG, "%s UNKNOWN", __func__); break;
        case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
        case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
        case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
        case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
        default: ESP_LOGW(TAG, "%s invalid type", __func__); break;
    }

    switch(signal.type)
    {
        case TASKS_SIG_AUDIO_STREAM_STARTED:
        case TASKS_SIG_AUDIO_STREAM_SUSPEND:
        case TASKS_SIG_AUDIO_DATA_SUFFICIENT:
            tasks_signal_send(signal);
            break;
        case TASKS_SIG_AUDIO_VOLUME:
            tasks_send_throttled_signal(signal, pdMS_TO_TICKS(200));
            break;
        default:
            ESP_LOGE(TAGE, "signal task: %d, signal type: %d", signal.aim_task, signal.type);
            PRINT_TRACE();
            break;
    }
}

void tasks_audio_data(const uint8_t *data, size_t size)
{
    if(audio_state == AUDIO_STATE_DROP)
    {
        if(pdTRUE == xSemaphoreTake(audio_semaphore, portMAX_DELAY))
        {
            dropped_bytes += size;
            xSemaphoreGive(audio_semaphore);
        }

        return;
    }

    if(pdTRUE == xRingbufferSend(audio_stream_ringbuf, data, size, 0))
    {
        if(audio_state == AUDIO_STATE_READY)
        {
            tasks_audio_state(AUDIO_STATE_PRELOAD);
        }

        if(audio_state == AUDIO_STATE_PRELOAD)
        {
            size_t item_size;
            vRingbufferGetInfo(audio_stream_ringbuf, NULL, NULL, NULL, NULL, &item_size);

            if(item_size >= RINGBUF_TRIGGER_LEVEL)
            {
                tasks_audio_state(AUDIO_STATE_PLAY);

                tasks_signal signal = {
                    .aim_task = TASKS_INST_AUDIO_PLAYER,
                    .type = TASKS_SIG_AUDIO_DATA_SUFFICIENT
                };
                tasks_message(signal);
            }
        }
    }
    else if(audio_state > AUDIO_STATE_STOP)
    {
        tasks_audio_state(AUDIO_STATE_DROP);
    }
}

static void tasks_throttler()
{
    TickType_t lastWakeTime;

    while(1)
    {
        lastWakeTime = xTaskGetTickCount();

        for(uint8_t task_i = 0; task_i < TASKS_INST_MAX; task_i++)
        {
            for(uint8_t sig_i = 0; sig_i < TASKS_SIG_MAX; sig_i++)
            {
                tasks_handle_throttled_signal(&throttled_signals[task_i][sig_i]);
            }
        }

        xTaskDelayUntil(&lastWakeTime, TASKS_THROTTLE_MIN_TIME);
    }
}

static void tasks_handle_throttled_signal(tasks_signal_throttled *signal)
{
    if(pdTRUE != xSemaphoreTake(throttler_semaphore, portMAX_DELAY)) return;

    bool need_send = false;

    if(signal->live)
    {
        // ESP_LOGW(TAG, "BEGIN %s", __func__);
        // throttled_signal_dumb(signal);

        if(signal->started)
        {
            if(signal->tick_left > TASKS_THROTTLE_MIN_TIME)
            {
                signal->tick_left -= TASKS_THROTTLE_MIN_TIME;
            }
            else
            {
                /* detect task still not processed previous signal,
                * to prevent task's mailbox queue overflow,
                * signal will sent after task acknowledged */
                if(!signal->need_acknowledge)
                {
                    if(signal->need_send)
                    {
                        need_send = true;
                        signal->need_send = false;
                        signal->need_acknowledge = true;
                    }

                    signal->tick_left = 0;
                    signal->live = false;
                    signal->started = false;
                }
            }
        }
        else
        {
            /* this provide min throttle time ellapsed */
            signal->started = true;
        }

        // ESP_LOGW(TAG, "END %s", __func__);
        // throttled_signal_dumb(signal);
    }

    xSemaphoreGive(throttler_semaphore);

    if(need_send) tasks_message(signal->waiting_signal);
}

static void tasks_send_throttled_signal(tasks_signal signal, TickType_t throttle_min_time)
{
    switch(signal.type)
    {
        case TASKS_SIG_UNKNOWN: ESP_LOGW(TAG, "%s UNKNOWN", __func__); break;
        case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
        case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
        case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
        case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
        default: ESP_LOGW(TAG, "%s invalid type", __func__); break;
    }

    bool immediate = false;
    tasks_signal_throttled *sig_throt = &throttled_signals[signal.aim_task][signal.type];
    ERR_CHECK_RETURN(pdTRUE != xSemaphoreTake(throttler_semaphore, portMAX_DELAY));
    // ESP_LOGW(TAG, "BEGIN %s", __func__);
    // throttled_signal_dumb(sig_throt);

    if(sig_throt->live)
    {
        sig_throt->need_send = true;
        sig_throt->waiting_signal = signal;
    }
    else
    {
        immediate = true;
        sig_throt->live = true;
        sig_throt->need_acknowledge = true;
        sig_throt->tick_left = throttle_min_time;
    }

    // ESP_LOGW(TAG, "END %s", __func__);
    // throttled_signal_dumb(sig_throt);
    xSemaphoreGive(throttler_semaphore);

    if(immediate) tasks_signal_send(signal);
}

static void tasks_audio_player()
{
    /* init audio peripheral */
    uint32_t total_dma_buf_size = I2S_DMA_FRAME_N * I2S_DMA_BUF_N;
    ach_control_init();
    ach_player_init(&total_dma_buf_size);
    tasks_audio_state(AUDIO_STATE_STOP);

    tasks_signal signal;
    TickType_t tick;

    while(1)
    {
        /* if audio stream ongoing, no block the audio playing */
        if(audio_state >= AUDIO_STATE_PLAY) tick = 0;
        else tick = portMAX_DELAY;

        if(pdTRUE == xQueueReceive(mails_audio_player, &signal, tick))
        {
            switch(signal.type)
            {
                case TASKS_SIG_AUDIO_STREAM_STARTED: {
                    ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__);

                    if(audio_state < AUDIO_STATE_READY)
                    {
                        tasks_audio_state(AUDIO_STATE_READY);
                        ach_player_start();
                    }
                    else if(audio_state == AUDIO_STATE_FLUSH)
                    {
                        tasks_audio_state(AUDIO_STATE_PLAY);
                    }

                    break;
                }
                case TASKS_SIG_AUDIO_STREAM_SUSPEND: {
                    ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__);

                    if(audio_state > AUDIO_STATE_STOP)
                    {
                        tasks_audio_state(AUDIO_STATE_FLUSH);
                    }

                    break;
                }
                case TASKS_SIG_AUDIO_DATA_SUFFICIENT: {
                    ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__);

                    if(audio_state == AUDIO_STATE_PRELOAD)
                    {
                        tasks_audio_state(AUDIO_STATE_PLAY);
                    }

                    break;
                }
                case TASKS_SIG_AUDIO_VOLUME: {
                    ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__);
                    ach_volume(signal.arg.audio_volume.volume);
                    break;
                }
                default:
                    ERR_BAD_CASE(signal.type, "%d");
                    break;
            }

            if(throttled_signals[TASKS_INST_AUDIO_PLAYER][signal.type].need_acknowledge)
            {
                ESP_LOGW(TAG, "signal acknowledged %d", signal.type);
                throttled_signals[TASKS_INST_AUDIO_PLAYER][signal.type].need_acknowledge = false;
            }
        }

        if(audio_state >= AUDIO_STATE_PLAY)
        {
            size_t item_size = 0;
            void *data = xRingbufferReceiveUpTo(audio_stream_ringbuf, &item_size, 0, total_dma_buf_size);

            if(item_size > 0)
            {
                ach_player_data(data, item_size);
                vRingbufferReturnItem(audio_stream_ringbuf, data);

                if(audio_state == AUDIO_STATE_DROP)
                {
                    vRingbufferGetInfo(audio_stream_ringbuf, NULL, NULL, NULL, NULL, &item_size);

                    if(item_size < RINGBUF_TRIGGER_LEVEL)
                    {
                        tasks_audio_state(AUDIO_STATE_PLAY);
                    }
                }
            }
            else
            {
                if(audio_state == AUDIO_STATE_FLUSH)
                {
                    tasks_audio_state(AUDIO_STATE_STOP);
                    ach_player_stop();
                }
                else
                {
                    tasks_audio_state(AUDIO_STATE_READY);
                }
            }
        }
    }
}

static void tasks_audio_state(audio_state_t new_state)
{
    switch(new_state)
    {
        case AUDIO_STATE_INIT: ESP_LOGI(TAG, "audio state: INIT"); break;
        case AUDIO_STATE_STOP: ESP_LOGI(TAG, "audio state: STOP"); break;
        case AUDIO_STATE_READY: ESP_LOGI(TAG, "audio state: READY"); break;
        case AUDIO_STATE_PRELOAD: ESP_LOGI(TAG, "audio state: PRELOAD"); break;
        case AUDIO_STATE_PLAY: ESP_LOGI(TAG, "audio state: PLAY"); break;
        case AUDIO_STATE_FLUSH: ESP_LOGI(TAG, "audio state: FLUSH"); break;
        case AUDIO_STATE_DROP: ESP_LOGI(TAG, "audio state: DROP"); break;
        default:
            ERR_BAD_CASE(new_state, "%d");
            return;
    }

    if(pdTRUE == xSemaphoreTake(audio_semaphore, portMAX_DELAY))
    {
        if(audio_state == AUDIO_STATE_DROP)
        {
            ESP_LOGI(TAG, "dropped bytes: %d", dropped_bytes);
        }

        if(new_state == AUDIO_STATE_DROP) dropped_bytes = 0;

        audio_state = new_state;
        xSemaphoreGive(audio_semaphore);
    }
    else PRINT_TRACE();
}

static void tasks_signal_send(tasks_signal signal)
{
    switch(signal.type)
    {
        case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
        case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
        case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
        case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
        default: break;
    }

    switch(signal.aim_task)
    {
        case TASKS_INST_AUDIO_PLAYER:
            ERR_CHECK(pdTRUE != xQueueSend(mails_audio_player, &signal, pdMS_TO_TICKS(10)));
            break;
        case TASKS_INST_AUDIO_DSP:
        case TASKS_INST_LIGHT:
        case TASKS_INST_HOTSPOT:
        default:
            ESP_LOGE(TAGE, "signal task: %d, signal type: %d", signal.aim_task, signal.type);
            PRINT_TRACE();
            break;
    }
}

static void tasks_lights()
{
    lights_test();

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// static void throttled_signal_dumb(tasks_signal_throttled *signal)
// {
//     printf("signal dump\n");
//     switch(signal->waiting_signal.aim_task)
//     {
//         case TASKS_INST_UNKNOWN: printf("task: UNKNOWN\n"); break;
//         case TASKS_INST_THROTTLER: printf("task: THROTTLER\n"); break;
//         case TASKS_INST_AUDIO_PLAYER: printf("task: AUDIO_PLAYER\n"); break;
//         case TASKS_INST_AUDIO_DSP: printf("task: AUDIO_DSP\n"); break;
//         case TASKS_INST_LIGHT: printf("task: LIGHT\n"); break;
//         case TASKS_INST_HOTSPOT: printf("task: HOTSPOT\n"); break;
//         default: printf("task has invalid value\n"); break;
//     }
//     switch(signal->waiting_signal.type)
//     {
//         case TASKS_SIG_UNKNOWN: printf("type: UNKNOWN\n"); break;
//         case TASKS_SIG_AUDIO_STREAM_STARTED: printf("type: AUDIO_STREAM_STARTED\n"); break;
//         case TASKS_SIG_AUDIO_STREAM_SUSPEND: printf("type: AUDIO_STREAM_SUSPEND\n"); break;
//         case TASKS_SIG_AUDIO_DATA_SUFFICIENT: printf("type: AUDIO_DATA_SUFFICIENT\n"); break;
//         case TASKS_SIG_AUDIO_VOLUME: printf("type: AUDIO_VOLUME\n"); break;
//         default: printf("type has invalid value\n"); break;
//     }
//     printf("tick left: %ld\n", signal->tick_left);
//     printf("live: %d\n", signal->live);
//     printf("started: %d\n", signal->started);
//     printf("need send: %d\n", signal->need_send);
//     printf("need acknowledge: %d\n\n", signal->need_acknowledge);
// }
