
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
#define STRIP_0_LED_N 50
#define STRIP_1_LED_N 266


static void mled_strip_init(uint8_t index, gpio_num_t pin, uint16_t pixel_n);
static size_t mled_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);
static esp_err_t mled_encode_reset(rmt_encoder_t *encoder);
static esp_err_t mled_encode_del(rmt_encoder_t *encoder);
static void rainbow();


static const char *TAG = LOG_COLOR("96") "MLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "MLED" LOG_COLOR_E;
static mled_strip channels[MLED_CHANNEL_N] = {0};


void mled_init()
{
    /* APB can configured with other value, so need check this */
    ERR_CHECK_RESET(MLED_CLOCK_HZ != clk_hal_apb_get_freq_hz());
    ESP_LOGI(TAG, "APB frequency matches");
    mled_strip_init(0, PIN_LED_STRIP_0, STRIP_0_LED_N);
    mled_strip_init(1, PIN_LED_STRIP_1, STRIP_1_LED_N);

    rainbow();
}

mled_pixels *mled_get_pixels(uint8_t strip_index)
{
    ERR_CHECK_RESET(MLED_CHANNEL_N <= strip_index);
    mled_strip *strip = &channels[strip_index];
    ERR_IF_NULL_RESET(strip->pixels.data);
    return &strip->pixels;
}

static void mled_strip_init(uint8_t index, gpio_num_t pin, uint16_t pixel_n)
{
    ERR_CHECK_RESET(MLED_CHANNEL_N <= index);
    ESP_LOGI(TAG, "init strip %d...", index);
    mled_strip *strip = &channels[index];
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = pin,
        .mem_block_symbols = RMT_CHANNEL_MEM_SIZE,
        .resolution_hz = MLED_CLOCK_HZ,
        .trans_queue_depth = 4,
    };
    ERR_CHECK_RESET(rmt_new_tx_channel(&tx_chan_config, &strip->tx_channel));
    ESP_LOGI(TAG, "new tx channel %d OK", index);

    strip->base = (rmt_encoder_t) {
        .encode = mled_encode,
        .reset = mled_encode_reset,
        .del = mled_encode_del
    };
    strip->data_sent = false;
    size_t mem_size = pixel_n * 3;
    mled_pixels *pixels = &strip->pixels;
    pixels->data = malloc(mem_size);
    ERR_IF_NULL_RESET(pixels->data);
    memset(pixels->data, 0, mem_size);
    pixels->data_size = mem_size;
    pixels->pixel_n = pixel_n;
    ESP_LOGI(TAG, "create pixels buf %d OK", index);
    mled_encode_chain_ws281x(strip);
    ESP_LOGI(TAG, "encode chain: WS281x %d OK", index);
    ERR_CHECK_RESET(rmt_enable(strip->tx_channel));
    ESP_LOGI(TAG, "init strip %d OK", index);
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

static void rainbow()
{
    mled_strip *strip;
    mled_pixels *pixels;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 1
        }
    };

    for(uint8_t i = 0; i < MLED_CHANNEL_N; i++)
    {
        strip = &channels[i];
        pixels = &strip->pixels;
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, pixels->data, pixels->data_size, &tx_config));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(strip->tx_channel, portMAX_DELAY));
        ESP_LOGI(TAG, "clear LED strip %d OK", i);
    }

    vTaskDelay(10);
    ESP_LOGI(TAG, "start LED RED...");

    {
        strip = &channels[0];
        pixels = &strip->pixels;

        for(int j = 0; j < STRIP_0_LED_N; j++)
        {
            switch(j%3)
            {
                case 0:
                    pixels->data[j * 3] = 60;
                    pixels->data[j * 3 + 1] = 0;
                    pixels->data[j * 3 + 2] = 0;
                    break;
                case 1:
                    pixels->data[j * 3] = 20;
                    pixels->data[j * 3 + 1] = 20;
                    pixels->data[j * 3 + 2] = 8;
                    break;
                case 2:
                    pixels->data[j * 3] = 0;
                    pixels->data[j * 3 + 1] = 40;
                    pixels->data[j * 3 + 2] = 0;
                    break;
            }
        }
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, pixels->data, pixels->data_size, &tx_config));

        strip = &channels[1];
        pixels = &strip->pixels;

        for(int j = 0; j < STRIP_1_LED_N; j++)
        {
            pixels->data[j * 3] = 10;
            pixels->data[j * 3 + 1] = 2;
        }
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, pixels->data, pixels->data_size, &tx_config));

        ERR_CHECK_RETURN(rmt_tx_wait_all_done(channels[0].tx_channel, portMAX_DELAY));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(channels[1].tx_channel, portMAX_DELAY));
    }
}
