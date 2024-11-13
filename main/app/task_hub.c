
#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#include "app_config.h"
#include "task_hub.h"
#include "app_tools.h"
#include "stereo_codec.h"
#include "bt_profiles.h"


#define RINGBUF_SIZE (40 * 1024)
#define RINGBUF_TRIGGER_LEVEL (16 * 1024)

enum {
    RINGBUF_STATE_INIT,
    RINGBUF_STATE_READY,
    RINGBUF_STATE_PLAY, /* ringbuffer is buffering incoming audio data, I2S is working */
    RINGBUF_STATE_PRELOAD, /* ringbuffer is buffering incoming audio data, I2S is waiting */
    RINGBUF_STATE_DROP /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
};


static void task_hub_I2S_process();
static void task_hub_I2C_process();


static const char *TAG = LOG_COLOR("91") "task" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("91") "task" LOG_COLOR_E;
/* handle of I2S task */
static TaskHandle_t i2s_task = NULL;
/* handle of ringbuffer for I2S */
static RingbufHandle_t i2s_ringbuf = NULL;
static SemaphoreHandle_t i2s_tx_semaphore = NULL;
static SemaphoreHandle_t i2c_bus_semaphore = NULL;
static uint8_t i2s_ringbuf_state = RINGBUF_STATE_INIT;
static size_t dropped_bytes = 0;

static uint8_t last_vol = 0;


void task_hub_tasks_create()
{
    i2c_bus_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RETURN(i2c_bus_semaphore);

    xTaskCreatePinnedToCore(task_hub_I2C_process, "I2C", 6000, NULL, 12, NULL, 1);
    task_hub_I2S_buf_init();
}

void task_hub_I2S_buf_init()
{
    ESP_LOGI(TAG, "I2S ringbuffer init...");
    i2s_tx_semaphore = xSemaphoreCreateBinary();
    ERR_IF_NULL_RETURN(i2s_tx_semaphore);

    i2s_ringbuf = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    ERR_IF_NULL_RETURN(i2s_ringbuf);

    i2s_ringbuf_state = RINGBUF_STATE_READY;
    ESP_LOGI(TAG, "ringbuf state changed: READY");
}

void task_hub_I2S_create()
{
    ERR_CHECK_RETURN(i2s_ringbuf_state != RINGBUF_STATE_READY);

    ESP_LOGI(TAG, "ringbuf state changed: PRELOAD");
    i2s_ringbuf_state = RINGBUF_STATE_PRELOAD;

    xTaskCreatePinnedToCore(task_hub_I2S_process, "I2STask", 8000, NULL, 11, &i2s_task, 1);
}

void task_hub_I2S_del()
{
    i2s_ringbuf_state = RINGBUF_STATE_INIT;
    ESP_LOGI(TAG, "ringbuf state changed: INIT");
    stereo_codec_I2S_stop();

    if(i2s_task)
    {
        vTaskDelete(i2s_task);
        i2s_task = NULL;
    }

    if(i2s_ringbuf)
    {
        vRingbufferDelete(i2s_ringbuf);
        i2s_ringbuf = NULL;
    }

    if(i2s_tx_semaphore)
    {
        vSemaphoreDelete(i2s_tx_semaphore);
        i2s_tx_semaphore = NULL;
    }

    task_hub_I2S_buf_init();
}

void task_hub_ringbuf_send(const uint8_t *data, size_t size)
{
    size_t buf_items_size = 0;
    BaseType_t done = pdFALSE;

    if(i2s_ringbuf_state == RINGBUF_STATE_DROP)
    {
        dropped_bytes += size;
        vRingbufferGetInfo(i2s_ringbuf, NULL, NULL, NULL, NULL, &buf_items_size);

        if(buf_items_size <= RINGBUF_TRIGGER_LEVEL)
        {
            ESP_LOGI(TAG, "ringbuf healed, state changed: PLAY");
            ESP_LOGI(TAG, "dropped bytes: %d", dropped_bytes);
            i2s_ringbuf_state = RINGBUF_STATE_PLAY;
        }

        return;
    }

    done = xRingbufferSend(i2s_ringbuf, (void *)data, size, 0);

    if(pdTRUE == done)
    {
        if(i2s_ringbuf_state == RINGBUF_STATE_PRELOAD)
        {
            vRingbufferGetInfo(i2s_ringbuf, NULL, NULL, NULL, NULL, &buf_items_size);

            if(buf_items_size >= RINGBUF_TRIGGER_LEVEL)
            {
                ESP_LOGI(TAG, "ringbuf triggered, state changed: PLAY");
                i2s_ringbuf_state = RINGBUF_STATE_PLAY;
                stereo_codec_I2S_enable_channel();

                if(pdFALSE == xSemaphoreGive(i2s_tx_semaphore))
                {
                    ESP_LOGE(TAGE, "i2s write semaphore give failed");
                }
            }
        }
    }
    else
    {
        ESP_LOGW(TAGE, "ringbuf overflowed, state changed: DROP");
        i2s_ringbuf_state = RINGBUF_STATE_DROP;
        dropped_bytes = 0;
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
        if(pdTRUE == xSemaphoreTake(i2s_tx_semaphore, portMAX_DELAY))
        {
            while(1)
            {
                item_size = 0;
                /* receive data from ringbuffer and write it to I2S DMA transmit buffer */
                data = (uint8_t *)xRingbufferReceiveUpTo(i2s_ringbuf, &item_size, pdMS_TO_TICKS(60), item_size_upto);

                if(item_size == 0)
                {
                    ESP_LOGI(TAG, "ringbuf underflowed, state changed: PRELOAD");
                    i2s_ringbuf_state = RINGBUF_STATE_PRELOAD;
                    stereo_codec_I2S_disable_channel();
                    break;
                }

                stereo_codec_I2S_write(data, item_size);
                vRingbufferReturnItem(i2s_ringbuf, data);
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
        if(pdTRUE == xSemaphoreTake(i2c_bus_semaphore, portMAX_DELAY))
        {
            uint8_t tmp = last_vol;
            // uint8_t tmp_in_percent = tmp * 100 / 0x7f;
            // ESP_LOGI(TAG, "start volume set: %d%%", tmp_in_percent);
            stereo_codec_set_volume(tmp);
            vTaskDelay(pdMS_TO_TICKS(500));
            // ESP_LOGI(TAG, "end volume set: %d%%", tmp_in_percent);
        }
    }

}

void task_hub_set_volume(uint8_t vol)
{
    last_vol = vol;
    xSemaphoreGive(i2c_bus_semaphore);
}
