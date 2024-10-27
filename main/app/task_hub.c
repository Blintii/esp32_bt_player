
#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#include "app_config.h"
#include "task_hub.h"
#include "app.h"
#include "stereo_codec.h"
#include "bt_gap.h"


#define RINGBUF_HIGHEST_WATER_LEVEL    (40 * 1024)
#define RINGBUF_PREFETCH_WATER_LEVEL   (16 * 1024)

enum {
    RINGBUFFER_MODE_INIT,
    RINGBUFFER_MODE_READY,
    RINGBUFFER_MODE_PROCESSING,    /* ringbuffer is buffering incoming audio data, I2S is working */
    RINGBUFFER_MODE_PREFETCHING,   /* ringbuffer is buffering incoming audio data, I2S is waiting */
    RINGBUFFER_MODE_DROPPING       /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
};


static bool bt_app_send_msg(bt_app_msg_t *msg);
static void task_hub_I2S_process();
static void task_hub_I2C_process();


static const char *TAG = LOG_COLOR("91") "task" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("91") "task" LOG_COLOR_E;
/* handle of I2S task */
static TaskHandle_t s_bt_i2s_task_handle = NULL;
/* handle of work queue */
static QueueHandle_t s_bt_app_task_queue = NULL;
/* handle of ringbuffer for I2S */
static RingbufHandle_t s_ringbuf_i2s = NULL;
static SemaphoreHandle_t s_i2s_write_semaphore = NULL;
static SemaphoreHandle_t s_i2c_semaphore = NULL;
static uint16_t ringbuffer_mode = RINGBUFFER_MODE_INIT;

static uint8_t last_vol = 0;


bool task_hub_bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len)
{
    bt_app_msg_t msg;
    memset(&msg, 0, sizeof(bt_app_msg_t));

    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return bt_app_send_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);
            return bt_app_send_msg(&msg);
        }
    }

    return false;
}

static bool bt_app_send_msg(bt_app_msg_t *msg)
{
    ERR_IF_NULL_RETURN_VAL(msg, false);

    /* send the message to work queue */
    ERR_CHECK_RETURN_VAL((pdTRUE != xQueueSend(s_bt_app_task_queue, msg, pdMS_TO_TICKS(10))), false);
    return true;
}

void task_hub_tasks_create()
{
    s_bt_app_task_queue = xQueueCreate(10, sizeof(bt_app_msg_t));
    ERR_IF_NULL_RETURN(s_bt_app_task_queue);

    s_i2c_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RETURN(s_i2c_semaphore);

    xTaskCreatePinnedToCore(task_hub_bt_app_process, "MainAppTask", 8000, NULL, 20, NULL, 1);
    xTaskCreatePinnedToCore(task_hub_I2C_process, "I2C", 8000, NULL, 24, NULL, 1);
    task_hub_I2S_buf_init();
}

void task_hub_bt_app_process()
{
    bt_app_msg_t msg;

    while(1)
    {
        /* receive message from work queue and handle it */
        if(pdTRUE == xQueueReceive(s_bt_app_task_queue, &msg, portMAX_DELAY))
        {
            if(msg.cb) msg.cb(msg.event, msg.param);

            if(msg.param) free(msg.param);
        }
    }
}

void task_hub_I2S_buf_init()
{
    ESP_LOGE(TAG, "I2S ringbuffer init...");
    s_i2s_write_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RETURN(s_i2s_write_semaphore);

    s_ringbuf_i2s = xRingbufferCreate(RINGBUF_HIGHEST_WATER_LEVEL, RINGBUF_TYPE_BYTEBUF);
    ERR_IF_NULL_RETURN(s_ringbuf_i2s);

    ringbuffer_mode = RINGBUFFER_MODE_READY;
    ESP_LOGE(TAG, "I2S ringbuffer ready!");
}

void task_hub_I2S_create()
{
    if(ringbuffer_mode != RINGBUFFER_MODE_READY)
    {
        ESP_LOGE(TAGE, "%s, I2S ringbuffer not ready", __func__);
        return;
    }

    ESP_LOGE(TAG, "ringbuffer data empty! mode changed: RINGBUFFER_MODE_PREFETCHING");
    ringbuffer_mode = RINGBUFFER_MODE_PREFETCHING;

    xTaskCreatePinnedToCore(task_hub_I2S_process, "I2STask", 8000, NULL, 22, &s_bt_i2s_task_handle, 1);
}

void task_hub_I2S_del()
{
    ringbuffer_mode = RINGBUFFER_MODE_INIT;
    stereo_codec_I2S_stop();

    if(s_bt_i2s_task_handle)
    {
        vTaskDelete(s_bt_i2s_task_handle);
        s_bt_i2s_task_handle = NULL;
    }

    if(s_ringbuf_i2s)
    {
        vRingbufferDelete(s_ringbuf_i2s);
        s_ringbuf_i2s = NULL;
    }

    if(s_i2s_write_semaphore)
    {
        vSemaphoreDelete(s_i2s_write_semaphore);
        s_i2s_write_semaphore = NULL;
    }

    task_hub_I2S_buf_init();
}

void task_hub_ringbuf_send(const uint8_t *data, size_t size)
{
    size_t buf_items_size = 0;
    BaseType_t done = pdFALSE;

    if(ringbuffer_mode == RINGBUFFER_MODE_DROPPING)
    {
        ESP_LOGW(TAGE, "ringbuffer is full, drop this packet!");
        vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &buf_items_size);

        if(buf_items_size <= RINGBUF_PREFETCH_WATER_LEVEL)
        {
            ESP_LOGE(TAG, "ringbuffer data decreased! mode changed: RINGBUFFER_MODE_PROCESSING");
            ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
        }

        return;
    }

    done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, 0);

    if(pdTRUE == done)
    {
        if(ringbuffer_mode == RINGBUFFER_MODE_PREFETCHING)
        {
            vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &buf_items_size);

            if(buf_items_size >= RINGBUF_PREFETCH_WATER_LEVEL)
            {
                ESP_LOGE(TAG, "ringbuffer data increased! mode changed: RINGBUFFER_MODE_PROCESSING");
                ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
                stereo_codec_I2S_enable_channel();

                if(pdFALSE == xSemaphoreGive(s_i2s_write_semaphore))
                {
                    ESP_LOGE(TAGE, "i2s write semaphore give failed");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(TAGE, "ringbuffer overflowed, ready to decrease data! mode changed: RINGBUFFER_MODE_DROPPING");
        ringbuffer_mode = RINGBUFFER_MODE_DROPPING;
    }
}

static void task_hub_I2S_process()
{
    stereo_codec_I2S_start();
    bt_gap_led_set_weak();

    uint8_t *data;
    size_t item_size;
    /**
     * The total length of DMA buffer of I2S is:
     * `dma_frame_num * dma_desc_num * i2s_channel_num * i2s_data_bit_width / 8`.
     * Transmit `dma_frame_num * dma_desc_num` bytes to DMA is trade-off.
     */
    const size_t item_size_upto = I2S_DMA_FRAME_N * I2S_DMA_BUF_N;

    while(1)
    {
        if(pdTRUE == xSemaphoreTake(s_i2s_write_semaphore, portMAX_DELAY))
        {
            while(1)
            {
                item_size = 0;
                /* receive data from ringbuffer and write it to I2S DMA transmit buffer */
                data = (uint8_t *)xRingbufferReceiveUpTo(s_ringbuf_i2s, &item_size, pdMS_TO_TICKS(60), item_size_upto);

                if(item_size == 0)
                {
                    ESP_LOGE(TAG, "ringbuffer underflowed! mode changed: RINGBUFFER_MODE_PREFETCHING");
                    ringbuffer_mode = RINGBUFFER_MODE_PREFETCHING;
                    stereo_codec_I2S_disable_channel();
                    break;
                }

                stereo_codec_I2S_write(data, item_size, portMAX_DELAY);
                vRingbufferReturnItem(s_ringbuf_i2s, data);
            }
        }
    }
}

static void task_hub_I2C_process()
{
    /* init audio peripheral */
    stereo_codec_control_init();

    while(1)
    {
        if(pdTRUE == xSemaphoreTake(s_i2c_semaphore, portMAX_DELAY))
        {
            uint8_t tmp = last_vol;
            uint8_t tmp_in_percent = tmp * 100 / 0x7f;
            ESP_LOGE(TAG, "start volume set: %d%%", tmp_in_percent);
            stereo_codec_set_volume(tmp);
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_LOGE(TAG, "end volume set: %d%%", tmp_in_percent);
        }
    }

}

void task_hub_set_volume(uint8_t vol)
{
    last_vol = vol;
    xSemaphoreGive(s_i2c_semaphore);
}
