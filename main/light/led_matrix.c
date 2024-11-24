
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_clk_tree.h"
#include "hal/clk_tree_hal.h"
#include "soc/soc_caps.h"

#include "app_config.h"
#include "app_tools.h"
#include "led_matrix.h"


#define STRIPS_N 2
#define RESOLUTION_HZ 80000000
#define DURATION_US_TO_CYC(us) ((RESOLUTION_HZ/1000000.0f)*us)
#define DURATION_NS_TO_CYC(ns) ((RESOLUTION_HZ/1000000000.0f)*ns)
#define RMT_MEM_SIZE (SOC_RMT_GROUPS*SOC_RMT_CHANNELS_PER_GROUP*SOC_RMT_MEM_WORDS_PER_CHANNEL)
#define RMT_CHANNEL_MEM_SIZE (RMT_MEM_SIZE/STRIPS_N)
/* the mled_strip structure layout allows to the structure can obtained
 * by using the address of the first element of the structure,
 * which we have that is the rmt_encoder_t */
#define GET_STRIP_FROM_BASE(encoder_base) (mled_strip*) encoder_base
#define STRIP_0_LED_N 50
#define STRIP_1_LED_N 266


typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    rmt_channel_handle_t tx_channel;
    bool need_reset;
    uint8_t *pixels;
    uint32_t pixels_size;
    uint16_t pixel_n;
} mled_strip;


static void strip_init(uint8_t index, gpio_num_t pin, uint16_t pixel_n);
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder);
static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder);
static void rainbow();
static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);


static const char *TAG = LOG_COLOR("95") "MLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "MLED" LOG_COLOR_E;
static mled_strip channels[STRIPS_N] = {0};
static const rmt_bytes_encoder_config_t encoder_default_config = {
    .bit0 = {
        .level0 = 1,
        .duration0 = DURATION_NS_TO_CYC(240), // T0H
        .level1 = 0,
        .duration1 = DURATION_NS_TO_CYC(600), // T0L
    },
    .bit1 = {
        .level0 = 1,
        .duration0 = DURATION_NS_TO_CYC(600), // T1H
        .level1 = 0,
        .duration1 = DURATION_NS_TO_CYC(240), // T1L
    },
    .flags.msb_first = 1 // WS2815 transfer bit order: G7...G0 R7...R0 B7...B0
};
static const rmt_symbol_word_t reset_code = {
    .level0 = 0,
    .duration0 = DURATION_US_TO_CYC(300),
    .level1 = 0,
    .duration1 = 1 // the zero duration used only internally as TX EOF flag by RMT
};


void led_strip_app(void)
{
    /* APB can configured with other value, so need check this */
    ERR_CHECK_RESET(RESOLUTION_HZ != clk_hal_apb_get_freq_hz());
    ESP_LOGI(TAG, "APB frequency matches");
    strip_init(0, PIN_LED_STRIP_0, STRIP_0_LED_N);
    strip_init(1, PIN_LED_STRIP_1, STRIP_1_LED_N);

    rainbow();
}

static void strip_init(uint8_t index, gpio_num_t pin, uint16_t pixel_n)
{
    ERR_CHECK_RESET(STRIPS_N <= index);
    ESP_LOGI(TAG, "init strip %d...", index);
    mled_strip *strip = &channels[index];
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = pin,
        .mem_block_symbols = RMT_CHANNEL_MEM_SIZE,
        .resolution_hz = RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ERR_CHECK_RESET(rmt_new_tx_channel(&tx_chan_config, &strip->tx_channel));
    ESP_LOGI(TAG, "new tx channel %d OK", index);

    strip->base = (rmt_encoder_t) {
        .encode = rmt_encode_led_strip,
        .reset = rmt_led_strip_encoder_reset,
        .del = rmt_del_led_strip_encoder
    };
    strip->need_reset = false;
    size_t mem_size = pixel_n * 3;
    strip->pixels = malloc(mem_size);
    ERR_IF_NULL_RESET(strip->pixels);
    memset(strip->pixels, 0, mem_size);
    strip->pixels_size = mem_size;
    strip->pixel_n = pixel_n;
    ESP_LOGI(TAG, "pixels buf %d OK", index);
    ERR_CHECK_RESET(rmt_new_bytes_encoder(&encoder_default_config, &strip->bytes_encoder));
    rmt_copy_encoder_config_t copy_encoder_config;
    ERR_CHECK_RESET(rmt_new_copy_encoder(&copy_encoder_config, &strip->copy_encoder));
    ESP_LOGI(TAG, "new encoders %d OK", index);
    ERR_CHECK_RESET(rmt_enable(strip->tx_channel));
    ESP_LOGI(TAG, "init strip %d OK", index);
}

static size_t IRAM_ATTR rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    rmt_encoder_handle_t bytes_encoder = strip->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = strip->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch(strip->need_reset)
    {
        case false: // send RGB data
            encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);

            if(session_state & RMT_ENCODING_COMPLETE)
            {
                strip->need_reset = true; // switch to next state when current encoding session finished
            }

            if(session_state & RMT_ENCODING_MEM_FULL)
            {
                state |= RMT_ENCODING_MEM_FULL;
                break;
            }
        // fall-through
        case true: // send reset code
            encoded_symbols += copy_encoder->encode(copy_encoder, channel, &reset_code, sizeof(reset_code), &session_state);

            if(session_state & RMT_ENCODING_COMPLETE)
            {
                strip->need_reset = false; // back to the initial encoding session
                state |= RMT_ENCODING_COMPLETE;
            }

            if(session_state & RMT_ENCODING_MEM_FULL)
            {
                state |= RMT_ENCODING_MEM_FULL;
            }
    }

    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    rmt_encoder_reset(strip->bytes_encoder);
    rmt_encoder_reset(strip->copy_encoder);
    strip->need_reset = false;
    ESP_LOGW(TAGE, "encoder reseted");
    return ESP_OK;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    mled_strip *strip = GET_STRIP_FROM_BASE(encoder);
    rmt_del_encoder(strip->bytes_encoder);
    rmt_del_encoder(strip->copy_encoder);
    ESP_LOGW(TAGE, "encoder deleted");
    return ESP_OK;
}

static void rainbow()
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    uint32_t value = 0;
    uint32_t tmp = 0;
    mled_strip *strip;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 1
        }
    };

    for(uint8_t i = 0; i < STRIPS_N; i++)
    {
        strip = &channels[i];
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, strip->pixels, strip->pixels_size, &tx_config));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(strip->tx_channel, portMAX_DELAY));
        ESP_LOGI(TAG, "clear LED strip %d OK", i);
    }

    vTaskDelay(10);
    ESP_LOGI(TAG, "start LED rainbow...");

    while(1)
    {
        strip = &channels[1];

        for(int j = 0; j < STRIP_1_LED_N; j++)
        {
            hue = j * 360 / STRIP_1_LED_N * 2 + start_rgb;
            value = start_rgb % 50;
            if(value >= 25) value = 50 - value;
            value += 2;
            hsv2rgb(hue, 100, value, &red, &green, &blue);
            strip->pixels[j * 3] = green;
            strip->pixels[j * 3 + 1] = red;
            strip->pixels[j * 3 + 2] = blue;
        }
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, strip->pixels, strip->pixels_size, &tx_config));

        strip = &channels[0];
        tmp = start_rgb % STRIP_0_LED_N;
        strip->pixels[tmp * 3] = 0;
        strip->pixels[tmp * 3 + 1] = 0;
        strip->pixels[tmp * 3 + 2] = 0;
        hsv2rgb(start_rgb, 100, 100, &red, &green, &blue);
        tmp = (start_rgb + 1) % STRIP_0_LED_N;
        strip->pixels[tmp * 3] = green;
        strip->pixels[tmp * 3 + 1] = red;
        strip->pixels[tmp * 3 + 2] = blue;
        ERR_CHECK_RETURN(rmt_transmit(strip->tx_channel, &strip->base, strip->pixels, strip->pixels_size, &tx_config));

        vTaskDelay(pdMS_TO_TICKS(500));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(channels[0].tx_channel, portMAX_DELAY));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(channels[1].tx_channel, portMAX_DELAY));
        start_rgb++;
    }
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}
