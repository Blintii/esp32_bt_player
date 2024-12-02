
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
#include "dsp.h"


typedef enum {
    AUDIO_STATE_INIT,
    AUDIO_STATE_STOP, // audio input stream off, play out off
    AUDIO_STATE_READY, // audio stream started, waiting first audio packet
    AUDIO_STATE_PRELOAD, // audio stream coming, buffered data exist, but not enough to play out
    AUDIO_STATE_PLAY, // audio stream coming, send buffered data to audio interface DMA memory
    AUDIO_STATE_DROP, // audio stream too fast, can't playing out all buffered data
    AUDIO_STATE_FLUSH // audio stream off, but buffer need play out remaining data
} audio_state_t;


static void tasks_throttler();
static void tasks_handle_throttled_signal(tasks_signal_throttled *signal);
static void tasks_send_throttled_signal(tasks_signal signal, TickType_t throttle_min_time);
static void tasks_audio_player();
static void tasks_audio_state(audio_state_t new_state);
static bool tasks_audio_stream_prepare();
static void tasks_audio_stream_terminate();
static void tasks_signal_send(tasks_signal signal);
static void tasks_dsp();
static void tasks_lights();
// static void throttled_signal_dumb(tasks_signal_throttled *signal);


static const char *TAG = LOG_COLOR("91") "TASK" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("91") "TASK" LOG_COLOR_E;
/* used for handling throttled signals */
static SemaphoreHandle_t throttler_semaphore = NULL;
/* used for make thread safe the audio state changeing */
static SemaphoreHandle_t audio_semaphore = NULL;
/* used for make thread safe the DSP internal ringbuf r/w */
static SemaphoreHandle_t dsp_in_semaphore = NULL;
/* used for make thread safe the DSP FFT result values r/w */
static SemaphoreHandle_t dsp_out_semaphore = NULL;
static QueueHandle_t mails_audio_player = NULL;
static RingbufHandle_t audio_stream_ringbuf = NULL;
static tasks_signal_throttled throttled_signals[TASKS_INST_MAX][TASKS_SIG_MAX] = {0}; // semaphored with throttler_semaphore
static audio_state_t audio_state = AUDIO_STATE_INIT; // semaphored with audio_semaphore
static size_t dropped_bytes = 0; // semaphored with audio_semaphore
static size_t rip_count = 0;
static size_t rip_sum = 0;


void tasks_create()
{
    ESP_LOGI(TAG, "init variables...");
    throttler_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(throttler_semaphore);
    xSemaphoreGive(throttler_semaphore);
    audio_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(audio_semaphore);
    xSemaphoreGive(audio_semaphore);
    dsp_in_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(dsp_in_semaphore);
    xSemaphoreGive(dsp_in_semaphore);
    dsp_out_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RESET(dsp_out_semaphore);
    xSemaphoreGive(dsp_out_semaphore);
    mails_audio_player = xQueueCreate(10, sizeof(tasks_signal));
    ERR_IF_NULL_RESET(mails_audio_player);
    ESP_LOGI(TAG, "init variables OK");

    ERR_CHECK_RESET(pdPASS != xTaskCreatePinnedToCore(tasks_throttler, "Throttler", 1984, NULL, 12, NULL, 1));
    ERR_CHECK_RESET(pdPASS != xTaskCreatePinnedToCore(tasks_audio_player, "Audio Player", 2304, NULL, 12, NULL, 1));
    ERR_CHECK_RESET(pdPASS != xTaskCreatePinnedToCore(tasks_dsp, "DSP", 2048, NULL, 12, NULL, 1));
    ERR_CHECK_RESET(pdPASS != xTaskCreatePinnedToCore(tasks_lights, "Lights", 2176, NULL, 12, NULL, 1));
    ESP_LOGI(TAG, "create tasks OK");
}

void tasks_message(tasks_signal signal)
{
    // switch(signal.type)
    // {
    //     case TASKS_SIG_UNKNOWN: ESP_LOGW(TAG, "%s UNKNOWN", __func__); break;
    //     case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
    //     case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
    //     case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
    //     case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
    //     default: ESP_LOGW(TAG, "%s invalid type", __func__); break;
    // }

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
    if(pdTRUE == xSemaphoreTake(dsp_in_semaphore, portMAX_DELAY))
    {
        dsp_new_data(data, size);
        xSemaphoreGive(dsp_in_semaphore);
    }
    else PRINT_TRACE();

    if(audio_state == AUDIO_STATE_DROP)
    {
        if(pdTRUE == xSemaphoreTake(audio_semaphore, portMAX_DELAY))
        {
            dropped_bytes += size;
            xSemaphoreGive(audio_semaphore);
        }
        else PRINT_TRACE();

        return;
    }

    if(audio_state < AUDIO_STATE_READY)
    {
        ESP_LOGE(TAGE, "audio state not ready when audio stream comes");
        return;
    }

    if(!audio_stream_ringbuf)
    {
        ESP_LOGE(TAGE, "audio stream ringbuffer is NULL");
        return;
    }

    bool drop = true;
    bool force_wakeup_notify = false;
    UBaseType_t buf_pos_free = 0,
        buf_pos_acquire = 0,
        buf_waiting = 0;
    vRingbufferGetInfo(audio_stream_ringbuf, &buf_pos_free, NULL, NULL, &buf_pos_acquire, &buf_waiting);
    /* free size calculatin method copied from ringbuf.c > prvGetCurMaxSizeByteBuf */
    BaseType_t free_size = buf_pos_free - buf_pos_acquire;
    if(free_size <= 0) free_size += AUDIO_BUF_LEN;

    if(size < free_size)
    {
        if(pdTRUE == xRingbufferSend(audio_stream_ringbuf, data, size, 0))
        {
            drop = false;
            buf_waiting += size;

            if(audio_state == AUDIO_STATE_READY)
            {
                tasks_audio_state(AUDIO_STATE_PRELOAD);
            }
        }
        else PRINT_TRACE();
    }

    if(drop)
    {
        if(audio_state == AUDIO_STATE_PRELOAD) force_wakeup_notify = true;

        tasks_audio_state(AUDIO_STATE_DROP);
        size_t actual_size = size < free_size ? size : free_size;
        /* size always have to a multiplication of
         * AUDIO_SAMPLE_BYTE_LEN * AUDIO_CHANNEL_N
         * (removing last 2 bit makes number divisible by 4) */
        actual_size &= ~3;

        if(pdTRUE != xRingbufferSend(audio_stream_ringbuf, data, actual_size, 0))
        {
            PRINT_TRACE();
            actual_size = 0;
        }

        if(pdTRUE == xSemaphoreTake(audio_semaphore, portMAX_DELAY))
        {
            dropped_bytes += size - actual_size;
            xSemaphoreGive(audio_semaphore);
        }
        else PRINT_TRACE();
    }
    else if(audio_state == AUDIO_STATE_PRELOAD)
    {
        if(buf_waiting >= AUDIO_BUF_TRIGGER_LEVEL)
        {
            tasks_audio_state(AUDIO_STATE_PLAY);
            force_wakeup_notify = true;
        }
    }

    if(force_wakeup_notify)
    {
        tasks_signal signal = {
            .aim_task = TASKS_INST_AUDIO_PLAYER,
            .type = TASKS_SIG_AUDIO_DATA_SUFFICIENT
        };
        tasks_message(signal);
    }
}

static void tasks_throttler()
{
    ESP_LOGI(TAG, "throttler started");
    TickType_t lastWakeTime;

    ESP_LOGI(TAG, "throttler enter infinite loop");
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
    // switch(signal.type)
    // {
    //     case TASKS_SIG_UNKNOWN: ESP_LOGW(TAG, "%s UNKNOWN", __func__); break;
    //     case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
    //     case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
    //     case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
    //     case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
    //     default: ESP_LOGW(TAG, "%s invalid type", __func__); break;
    // }

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
    ESP_LOGI(TAG, "audio player started");
    tasks_signal signal;
    TickType_t tick;
    uint32_t total_dma_buf_size = I2S_DMA_BUF_SIZE * I2S_DMA_BUF_N;
    /* init audio peripheral */
    ach_control_init();
    ach_player_init(&total_dma_buf_size);
    tasks_audio_state(AUDIO_STATE_STOP);

    ESP_LOGI(TAG, "audio player enter infinite loop");
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
                    ESP_LOGI(TAG, "%s AUDIO_STREAM_STARTED", __func__);

                    if(audio_state < AUDIO_STATE_READY)
                    {
                        if(tasks_audio_stream_prepare()) tasks_audio_state(AUDIO_STATE_READY);
                    }
                    else if(audio_state == AUDIO_STATE_FLUSH)
                    {
                        tasks_audio_state(AUDIO_STATE_PLAY);
                    }

                    break;
                }
                case TASKS_SIG_AUDIO_STREAM_SUSPEND: {
                    ESP_LOGI(TAG, "%s AUDIO_STREAM_SUSPEND", __func__);

                    if(audio_state > AUDIO_STATE_STOP)
                    {
                        tasks_audio_state(AUDIO_STATE_FLUSH);
                    }

                    break;
                }
                case TASKS_SIG_AUDIO_DATA_SUFFICIENT: {
                    // ESP_LOGI(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__);
                    /* just wake up from xQueueReceive to start playing audio */
                    break;
                }
                case TASKS_SIG_AUDIO_VOLUME: {
                    // ESP_LOGI(TAG, "%s AUDIO_VOLUME", __func__);
                    ach_volume(signal.arg.audio_volume.volume);
                    break;
                }
                default:
                    ERR_BAD_CASE(signal.type, "%d");
                    break;
            }

            if(throttled_signals[TASKS_INST_AUDIO_PLAYER][signal.type].need_acknowledge)
            {
                // ESP_LOGW(TAG, "signal acknowledged %d", signal.type);
                throttled_signals[TASKS_INST_AUDIO_PLAYER][signal.type].need_acknowledge = false;
            }
        }

        if(audio_state >= AUDIO_STATE_PLAY)
        {
            size_t item_size;
            void *data;
            size_t flush_cnt = 0;
            bool more_data_left = true;

            do
            {
                item_size = 0;
                data = xRingbufferReceiveUpTo(audio_stream_ringbuf, &item_size, 0, AUDIO_BUF_RECEIVE_SIZE);

                if(item_size)
                {
                    ach_player_data(data, item_size);
                    vRingbufferReturnItem(audio_stream_ringbuf, data);
                }
                else
                {
                    more_data_left = false;
                    break;
                }

                if(flush_cnt < AUDIO_BUF_MAX_FLUSH_N) flush_cnt++;
                else break;
            }
            while(item_size);

            if(more_data_left)
            {
                if(audio_state == AUDIO_STATE_DROP)
                {
                    vRingbufferGetInfo(audio_stream_ringbuf, NULL, NULL, NULL, NULL, &item_size);

                    if(item_size < AUDIO_BUF_TRIGGER_LEVEL)
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
                    tasks_audio_stream_terminate();
                }
                else
                {
                    /* halt this task instead of continous ringbuf polling for data
                     * if new data comes, have to wake up this task with any task message,
                     * TASKS_SIG_AUDIO_DATA_SUFFICIENT signal can be used for this purpose */
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
        case AUDIO_STATE_READY: /*ESP_LOGI(TAG, "audio state: READY");*/ break;
        case AUDIO_STATE_PRELOAD: /*ESP_LOGI(TAG, "audio state: PRELOAD");*/ break;
        case AUDIO_STATE_PLAY: /*ESP_LOGI(TAG, "audio state: PLAY");*/ break;
        case AUDIO_STATE_FLUSH: ESP_LOGI(TAG, "audio state: FLUSH"); break;
        case AUDIO_STATE_DROP: ESP_LOGE(TAGE, "audio state: DROP"); break;
        default:
            ERR_BAD_CASE(new_state, "%d");
            return;
    }

    if(pdTRUE == xSemaphoreTake(audio_semaphore, portMAX_DELAY))
    {
        if(audio_state == AUDIO_STATE_DROP)
        {
            ESP_LOGE(TAGE, "dropped bytes: %d", dropped_bytes);
        }

        if(new_state == AUDIO_STATE_DROP) dropped_bytes = 0;

        audio_state = new_state;
        xSemaphoreGive(audio_semaphore);
    }
    else PRINT_TRACE();
}

static bool tasks_audio_stream_prepare()
{
    bool ret = false;

    if(audio_stream_ringbuf) ESP_LOGW(TAG, "audio stream not terminated properly before");
    else audio_stream_ringbuf = xRingbufferCreate(AUDIO_BUF_LEN, RINGBUF_TYPE_BYTEBUF);

    if(audio_stream_ringbuf)
    {
        if(pdTRUE == xSemaphoreTake(dsp_in_semaphore, portMAX_DELAY))
        {
            if(pdTRUE == xSemaphoreTake(dsp_out_semaphore, portMAX_DELAY))
            {
                list_tasks_stack_info();

                if(dsp_fft_buf_create())
                {
                    ach_player_start();
                    ret = true;
                    ESP_LOGI(TAG, "audio stream prepared");
                }
                else dsp_fft_buf_del();

                xSemaphoreGive(dsp_out_semaphore);
                list_tasks_stack_info();
            }
            else PRINT_TRACE();

            xSemaphoreGive(dsp_in_semaphore);
        }
        else PRINT_TRACE();
    }
    else PRINT_TRACE();

    if(!ret)
    {
        ESP_LOGE(TAGE, "audio stream prepare fail cleanup");
        vRingbufferDelete(audio_stream_ringbuf);
        audio_stream_ringbuf = NULL;
    }

    return ret;
}

static void tasks_audio_stream_terminate()
{
    vRingbufferDelete(audio_stream_ringbuf);
    audio_stream_ringbuf = NULL;
    ach_player_stop();

    if(pdTRUE == xSemaphoreTake(dsp_in_semaphore, portMAX_DELAY))
    {
        if(pdTRUE == xSemaphoreTake(dsp_out_semaphore, portMAX_DELAY))
        {
            dsp_fft_buf_del();
            ESP_LOGI(TAG, "audio stream terminated");
            xSemaphoreGive(dsp_out_semaphore);
        }
        else PRINT_TRACE();

        xSemaphoreGive(dsp_in_semaphore);
    }
    else PRINT_TRACE();
}

static void tasks_signal_send(tasks_signal signal)
{
    // switch(signal.type)
    // {
    //     case TASKS_SIG_AUDIO_STREAM_STARTED: ESP_LOGW(TAG, "%s AUDIO_STREAM_STARTED", __func__); break;
    //     case TASKS_SIG_AUDIO_STREAM_SUSPEND: ESP_LOGW(TAG, "%s AUDIO_STREAM_SUSPEND", __func__); break;
    //     case TASKS_SIG_AUDIO_DATA_SUFFICIENT: ESP_LOGW(TAG, "%s AUDIO_DATA_SUFFICIENT", __func__); break;
    //     case TASKS_SIG_AUDIO_VOLUME: ESP_LOGW(TAG, "%s AUDIO_VOLUME", __func__); break;
    //     default: break;
    // }

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

static void tasks_dsp()
{
    ESP_LOGI(TAG, "dsp started");
    TickType_t lastWakeTime;

    ESP_LOGI(TAG, "dsp enter infinite loop");
    while(1)
    {
        lastWakeTime = xTaskGetTickCount();

        /* DSP buffers only available from at least READY audio state */
        if(audio_state >= AUDIO_STATE_READY)
        {
            if(pdTRUE == xSemaphoreTake(dsp_in_semaphore, portMAX_DELAY))
            {
                /* before fft need latest audio data be copied to work buf */
                dsp_work_buf_init();
                xSemaphoreGive(dsp_in_semaphore);
                dsp_fft_do();

                if(pdTRUE == xSemaphoreTake(dsp_out_semaphore, portMAX_DELAY))
                {
                    dsp_fft_finalize();
                    xSemaphoreGive(dsp_out_semaphore);
                }
                else PRINT_TRACE();
            }
            else PRINT_TRACE();
        }

        if(pdTRUE != xTaskDelayUntil(&lastWakeTime, TASKS_DSP_MIN_TIME)) rip_sum++;

        rip_count++;

        if(1000 < rip_count)
        {
            ESP_LOGW(TAG, LOG_COLOR_W"DSP skipped %d tick under %d times", rip_sum, rip_count);
            rip_count = 0;
            rip_sum = 0;
        }
    }
}

#include "string.h"

static void tasks_lights()
{
    ESP_LOGI(TAG, "lights started");
    lights_set_strip_size(0, 150);
    lights_set_strip_size(1, 150);
    ESP_LOGI(TAG, "strips inited");
    mled_rgb_order colors = {.i_r = 1, .i_g = 0, .i_b = 2};
    mled_channels[0].rgb_order = colors;
    mled_channels[1].rgb_order = colors;
    lights_shader *shader;
    vTaskDelay(pdMS_TO_TICKS(3333));
    list_tasks_stack_info();
    lights_zone_chain *zone = lights_new_zone(0, 22);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {30, 1, 0.015f},
            {20, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(0, 22);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {20, 1, 0.015f},
            {10, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(0, 55);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {10, 1, 0.015f},
            {0, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(0, 51);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FFT;
        color_hsl pattern[] = {
            {0, 1, 0.5f},
            {20, 1, 0.5f},
            {165, 1, 0.5f},
            {310, 1, 0.5f},
            {340, 1, 0.5f}
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fft = (lights_shader_cfg_fft) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl),
            .intensity = 100.0f,
            .is_right = false,
            .mirror = true
        };
        lights_shader_init_fft(zone);
        shader->need_render = true;
    }
    zone = lights_new_zone(1, 22);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {170, 1, 0.015f},
            {180, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(1, 22);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {180, 1, 0.015f},
            {190, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(1, 55);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FADE;
        color_hsl pattern[] = {
            {190, 1, 0.015f},
            {200, 1, 0.015f},
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fade = (lights_shader_cfg_fade) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl)
        };
        shader->need_render = true;
    }
    zone = lights_new_zone(1, 51);
    if(zone)
    {
        shader = &zone->shader;
        shader->type = SHADER_FFT;
        color_hsl pattern[] = {
            {0, 1, 0.5f},
            {20, 1, 0.5f},
            {165, 1, 0.5f},
            {310, 1, 0.5f},
            {340, 1, 0.5f}
        };
        color_hsl *buf = (color_hsl*) malloc(sizeof(pattern));
        memcpy(buf, pattern, sizeof(pattern));
        shader->cfg.shader_fft = (lights_shader_cfg_fft) {
            .colors = buf,
            .color_n = sizeof(pattern)/sizeof(color_hsl),
            .intensity = 100.0f,
            .is_right = true,
            .mirror = true
        };
        lights_shader_init_fft(zone);
        shader->need_render = true;
    }

    TickType_t lastWakeTime;
    list_tasks_stack_info();
    ESP_LOGI(TAG, "lights enter infinite loop");
    while(1)
    {
        lastWakeTime = xTaskGetTickCount();

        if(pdTRUE == xSemaphoreTake(dsp_out_semaphore, portMAX_DELAY))
        {
            lights_main();
            xSemaphoreGive(dsp_out_semaphore);
        }
        else PRINT_TRACE();

        xTaskDelayUntil(&lastWakeTime, TASKS_LIGHTS_MIN_TIME);
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
