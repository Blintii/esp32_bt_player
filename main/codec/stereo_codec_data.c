
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2s_std.h"

#include "main.h"
#include "stereo_codec.h"


i2s_chan_handle_t tx_chan = NULL;


void stereo_codec_I2S_start()
{
    i2s_chan_config_t chan_cfg = {
        .id = I2S_PERIPH_NUM,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = I2S_DMA_BUF_N,
        .dma_frame_num = I2S_DMA_FRAME_N,
        .auto_clear = true
    };

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 44100,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = PIN_I2S_BCLK,
            .ws = PIN_I2S_WS,
            .dout = PIN_I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            }
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
}

void stereo_codec_I2S_stop()
{
    i2s_channel_disable(tx_chan);
    i2s_del_channel(tx_chan);
}

void stereo_codec_I2S_write(const void *src, size_t size, uint32_t timeout_ms)
{
    if(ESP_OK != i2s_channel_write(tx_chan, src, size, NULL, timeout_ms))
    {
        ESP_LOGE(LOG_STEREO_CODEC, "I2S channel write failed");
    }
}

void stereo_codec_I2S_enable_channel()
{
    ESP_LOGW(LOG_STEREO_CODEC, "I2S channel STARTED");
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void stereo_codec_I2S_disable_channel()
{
    ESP_LOGW(LOG_STEREO_CODEC, "I2S channel STOPPED");
    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
}
