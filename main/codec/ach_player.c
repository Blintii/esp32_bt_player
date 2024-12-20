
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "app_tools.h"
#include "ach.h"


static const char *TAG = LOG_COLOR("95") "CODEC" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "CODEC" LOG_COLOR_E;
static i2s_chan_handle_t tx_chan = NULL;


void ach_player_init(uint32_t *total_dma_buf_size)
{
    i2s_chan_config_t chan_cfg = {
        .id = I2S_PERIPH_NUM,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = I2S_DMA_BUF_N,
        .dma_frame_num = I2S_DMA_BUF_SIZE,
        .auto_clear = true
    };

    ERR_CHECK_RESET(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 44100,
            /* technical reference: the analog PLL output clock source APLL_CLK
               must be used to acquire highly accurate I2Sn_CLK and BCK */
            .clk_src = I2S_CLK_SRC_APLL,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(AUDIO_SAMPLE_BIT_LEN, I2S_SLOT_MODE),
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

    ERR_CHECK_RESET(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ERR_CHECK_RESET(gpio_set_drive_capability(PIN_I2S_BCLK, GPIO_DRIVE_CAP_0));
    ERR_CHECK_RESET(gpio_set_drive_capability(PIN_I2S_WS, GPIO_DRIVE_CAP_0));
    ERR_CHECK_RESET(gpio_set_drive_capability(PIN_I2S_DOUT, GPIO_DRIVE_CAP_0));
    i2s_chan_info_t info;
    i2s_channel_get_info(tx_chan, &info);
    ESP_LOGI(TAG, "I2S channel init OK with buf size: %ld", info.total_dma_buf_size);
    *total_dma_buf_size = info.total_dma_buf_size;
}

void ach_player_data(const void *src, size_t size)
{
    size_t done = 0;
    esp_err_t err = i2s_channel_write(tx_chan, src, size, &done, 30);

    if((ESP_OK != err) || (size != done))
    {
        ESP_LOGE(TAGE, "I2S channel write: %s, %d/%d", esp_err_to_name(err), done, size);
    }
}

void ach_player_start()
{
    ESP_LOGW(TAG, "I2S channel STARTED");
    ach_unmute();
    i2s_channel_enable(tx_chan);
}

void ach_player_stop()
{
    ESP_LOGW(TAG, "I2S channel STOPPED");
    i2s_channel_disable(tx_chan);
    ach_mute();
}
