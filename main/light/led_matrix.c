
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

#include "app_config.h"
#include "app_tools.h"
#include "led_matrix.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      PIN_LED_STRIP_1

#define EXAMPLE_LED_NUMBERS         95


typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;


static const char *TAG = LOG_COLOR("95") "MLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "MLED" LOG_COLOR_E;
static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];
rmt_led_strip_encoder_t led_strip_encoder;


static void rmt_new_led_strip_encoder();


static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_encoder_handle_t bytes_encoder = led_strip_encoder.bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_strip_encoder.copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_strip_encoder.state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_strip_encoder.state = 1; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space for encoding artifacts
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_strip_encoder.reset_code,
                                                sizeof(led_strip_encoder.reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_strip_encoder.state = RMT_ENCODING_RESET; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space for encoding artifacts
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_del_encoder(led_strip_encoder.bytes_encoder);
    rmt_del_encoder(led_strip_encoder.copy_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_encoder_reset(led_strip_encoder.bytes_encoder);
    rmt_encoder_reset(led_strip_encoder.copy_encoder);
    led_strip_encoder.state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static void rmt_new_led_strip_encoder()
{
    led_strip_encoder.base.encode = rmt_encode_led_strip;
    led_strip_encoder.base.del = rmt_del_led_strip_encoder;
    led_strip_encoder.base.reset = rmt_led_strip_encoder_reset;
    // different led strip might have its own timing requirements, following parameter is for WS2812
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 3, // T0H=0.3us
            .level1 = 0,
            .duration1 = 9, // T0L=0.9us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 9, // T1H=0.9us
            .level1 = 0,
            .duration1 = 3, // T1L=0.3us
        },
        .flags.msb_first = 1 // WS2812 transfer bit order: G7...G0R7...R0B7...B0
    };
    ERR_CHECK_RESET(rmt_new_bytes_encoder(&bytes_encoder_config, &led_strip_encoder.bytes_encoder));
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ERR_CHECK_RESET(rmt_new_copy_encoder(&copy_encoder_config, &led_strip_encoder.copy_encoder));

    uint32_t reset_ticks = 3000 / 2; // reset code duration 300us
    led_strip_encoder.reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
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

void led_strip_app(void)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 512, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 32, // set the number of transactions that can be pending in the background
    };
    ERR_CHECK_RESET(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    rmt_new_led_strip_encoder();
    rmt_encoder_handle_t encoder_handle = &led_strip_encoder.base;

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ERR_CHECK_RESET(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    while (1) {
        for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++) {
            // Build RGB pixels
            hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
            led_strip_hsv2rgb(hue, 100, 8, &red, &green, &blue);
            led_strip_pixels[j * 3 + 0] = green;
            led_strip_pixels[j * 3 + 1] = blue;
            led_strip_pixels[j * 3 + 2] = red;
        }
        // Flush RGB values to LEDs
        ERR_CHECK_RETURN(rmt_transmit(led_chan, encoder_handle, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ERR_CHECK_RETURN(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(2);
        start_rgb++;
    }
}
