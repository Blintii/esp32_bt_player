
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "esp_clk_tree.h"
#include "hal/clk_tree_hal.h"
#include "soc/soc_caps.h"

#include "app_config.h"
#include "app_tools.h"
#include "led_matrix.h"


#define RMT_MEM_SIZE (SOC_RMT_GROUPS*SOC_RMT_CHANNELS_PER_GROUP*SOC_RMT_MEM_WORDS_PER_CHANNEL)
#define RMT_CHANNEL_MEM_SIZE (RMT_MEM_SIZE/MLED_CHANNEL_N)
/* the mled_strip structure layout allows to the structure can obtained
 * by using the address of the first element of the structure,
 * which we have that is the rmt_encoder_t */
#define GET_STRIP_FROM_BASE(encoder_base) (mled_strip*) encoder_base


static void mled_strip_init(size_t index, gpio_num_t pin);
static size_t mled_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);
static esp_err_t mled_encode_reset(rmt_encoder_t *encoder);
static esp_err_t mled_encode_del(rmt_encoder_t *encoder);


static const char *TAG = LOG_COLOR("96") "MLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "MLED" LOG_COLOR_E;
mled_strip mled_channels[MLED_CHANNEL_N] = {0};


void mled_init()
{
    /* APB can configured with other value, so need check this */
    ERR_CHECK_RESET(MLED_CLOCK_HZ != clk_hal_apb_get_freq_hz());
    mled_strip_init(0, PIN_LED_STRIP_0);
    mled_strip_init(1, PIN_LED_STRIP_1);
}

void mled_set_size(mled_strip *strip, size_t pixel_n)
{
    size_t mem_size = pixel_n * 3;
    mled_pixels *pixels = &strip->pixels;

    if(pixels->data)
    {
        ESP_LOGW(TAG, "strip already has pixels buf, freeing %d...", pixels->pixel_n);
        free(pixels->data);
        pixels->pixel_n = 0;
        pixels->data_size = 0;
    }

    pixels->data = malloc(mem_size);
    ERR_IF_NULL_RESET(pixels->data);
    memset(pixels->data, 0, mem_size);
    pixels->data_size = mem_size;
    pixels->pixel_n = pixel_n;
    ESP_LOGI(TAG, "set pixels buf size %d OK", pixel_n);
}

void mled_update(mled_strip *strip)
{
    mled_pixels *pixels = &strip->pixels;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 1
        }
    };
    ERR_CHECK(rmt_transmit(strip->tx_channel, &strip->base, pixels->data, pixels->data_size, &tx_config));
}

static void mled_strip_init(size_t index, gpio_num_t pin)
{
    ERR_CHECK_RESET(MLED_CHANNEL_N <= index);
    mled_strip *strip = &mled_channels[index];
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = pin,
        .mem_block_symbols = RMT_CHANNEL_MEM_SIZE,
        .resolution_hz = MLED_CLOCK_HZ,
        .trans_queue_depth = 4,
    };
    ERR_CHECK_RESET(rmt_new_tx_channel(&tx_chan_config, &strip->tx_channel));
    strip->base = (rmt_encoder_t) {
        .encode = mled_encode,
        .reset = mled_encode_reset,
        .del = mled_encode_del
    };
    strip->data_sent = false;
    mled_encode_chain_ws281x(strip);
    ERR_CHECK_RESET(rmt_enable(strip->tx_channel));
    ESP_LOGI(TAG, "init strip %d as WS281x OK", index);
}

static size_t IRAM_ATTR mled_encode(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    ERR_CHECK_RESET(strip->tx_channel != tx_channel);
    rmt_encoder_t *payload_handler = strip->payload_handler;
    rmt_encoder_t *eof_handler = strip->eof_handler;
    rmt_encode_state_t state_chain = RMT_ENCODING_RESET;
    rmt_encode_state_t state_own = RMT_ENCODING_RESET;
    size_t byte_cnt = 0;

    /* data can send after previous EOF already sent */
    if(!strip->data_sent)
    {
        byte_cnt += payload_handler->encode(payload_handler, tx_channel, primary_data, data_size, &state_chain);

        if(state_chain & RMT_ENCODING_COMPLETE)
        {
            strip->data_sent = true;
        }

        if(state_chain & RMT_ENCODING_MEM_FULL)
        {
            /* forward sub encoder state in encoding chain */
            state_own |= RMT_ENCODING_MEM_FULL;
        }
    }

    /* then complete data sent, need send reset code next */
    if(strip->data_sent)
    {
        byte_cnt += eof_handler->encode(eof_handler, tx_channel, strip->reset_code, strip->reset_code_size, &state_chain);

        if(state_chain & RMT_ENCODING_COMPLETE)
        {
            strip->data_sent = false;
            state_own |= RMT_ENCODING_COMPLETE;
        }

        if(state_chain & RMT_ENCODING_MEM_FULL)
        {
            /* forward sub encoder state in encoding chain */
            state_own |= RMT_ENCODING_MEM_FULL;
        }
    }

    *ret_state = state_own;
    return byte_cnt;
}

static esp_err_t mled_encode_reset(rmt_encoder_t *encoder)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    rmt_encoder_reset(strip->payload_handler);
    rmt_encoder_reset(strip->eof_handler);
    strip->data_sent = false;
    ESP_LOGW(TAGE, "encoder reseted");
    return ESP_OK;
}

static esp_err_t mled_encode_del(rmt_encoder_t *encoder)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    rmt_del_encoder(strip->payload_handler);
    rmt_del_encoder(strip->eof_handler);
    ESP_LOGW(TAGE, "encoder deleted");
    return ESP_OK;
}
