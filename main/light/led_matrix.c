
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_clk_tree.h"
#include "hal/clk_tree_hal.h"

#include "app_config.h"
#include "app_tools.h"
#include "led_matrix.h"


#define RESOLUTION_HZ 80000000
#define STRIP_LED_N 266
#define DURATION_US_TO_CYC(us) (RESOLUTION_HZ/1000000*us)


typedef struct {
    rmt_encoder_t base;
    rmt_encoder_handle_t bytes_encoder;
    rmt_encoder_handle_t copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;


static void rmt_new_led_strip_encoder();
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder);
static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder);
// static bool tx_done_callback(rmt_channel_handle_t tx_chan, const rmt_tx_done_event_data_t *edata, void *user_ctx);
static void rainbow();
static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);


static const char *TAG = LOG_COLOR("95") "MLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "MLED" LOG_COLOR_E;
static uint8_t led_strip_pixels[STRIP_LED_N * 3];
static rmt_channel_handle_t led_chan = NULL;
static rmt_led_strip_encoder_t led_strip_encoder = {
    .base = {
        .encode = rmt_encode_led_strip,
        .reset = rmt_led_strip_encoder_reset,
        .del = rmt_del_led_strip_encoder
    },
    .reset_code = {
        .level0 = 0,
        .duration0 = DURATION_US_TO_CYC(300),
        .level1 = 0,
        .duration1 = 0,
    }
};
// different led strip might have its own timing requirements, following parameter is for WS2812
static const rmt_bytes_encoder_config_t encoder_default_config = {
    .bit0 = {
        .level0 = 1,
        .duration0 = 24, // T0H=0.3us
        .level1 = 0,
        .duration1 = 72, // T0L=0.9us
    },
    .bit1 = {
        .level0 = 1,
        .duration0 = 72, // T1H=0.9us
        .level1 = 0,
        .duration1 = 24, // T1L=0.3us
    },
    .flags.msb_first = 1 // WS2812 transfer bit order: G7...G0R7...R0B7...B0
};


void led_strip_app(void)
{
    /* APB can configured with other value, so need check this */
    ERR_CHECK_RESET(RESOLUTION_HZ != clk_hal_apb_get_freq_hz());

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = PIN_LED_STRIP_1,
        .mem_block_symbols = 512, // increase the block size can make the LED less flickering
        .resolution_hz = RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ERR_CHECK_RESET(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    ESP_LOGI(TAG, "new TX channel OK");

    rmt_new_led_strip_encoder();
    ESP_LOGI(TAG, "new led strip encoder OK");
    // rmt_tx_event_callbacks_t cbs = {
    //     .on_trans_done = tx_done_callback
    // };
    // ERR_CHECK(rmt_tx_register_event_callbacks(led_chan, &cbs, NULL));
    ERR_CHECK_RESET(rmt_enable(led_chan));
    ESP_LOGI(TAG, "RMT enable OK");

    rainbow();
}

static void rmt_new_led_strip_encoder()
{
    ERR_CHECK_RESET(rmt_new_bytes_encoder(&encoder_default_config, &led_strip_encoder.bytes_encoder));
    rmt_copy_encoder_config_t copy_encoder_config;
    ERR_CHECK_RESET(rmt_new_copy_encoder(&copy_encoder_config, &led_strip_encoder.copy_encoder));
}

static size_t IRAM_ATTR rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
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
                break;
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
            }
    }

    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_encoder_reset(led_strip_encoder.bytes_encoder);
    rmt_encoder_reset(led_strip_encoder.copy_encoder);
    led_strip_encoder.state = 0;
    ESP_LOGW(TAGE, "encoder reseted");
    return ESP_OK;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_del_encoder(led_strip_encoder.bytes_encoder);
    rmt_del_encoder(led_strip_encoder.copy_encoder);
    ESP_LOGW(TAGE, "encoder deleted");
    return ESP_OK;
}

// static bool IRAM_ATTR tx_done_callback(rmt_channel_handle_t tx_chan, const rmt_tx_done_event_data_t *edata, void *user_ctx)
// {
//     return false;
// }

static void rainbow()
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint32_t value;
    rmt_encoder_handle_t encoder_handle = &led_strip_encoder.base;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
            .queue_nonblocking = 1
        }
    };
    rmt_bytes_encoder_config_t try_config = {
        .bit0 = {
            .level0 = 1,
            .level1 = 0,
        },
        .bit1 = {
            .level0 = 1,
            .level1 = 0,
        },
        .flags.msb_first = 1
    };
    int master = 1;
    int slave = 0;
    bool run = true;
    uint16_t tmp = 0;
    bool dec = false;
    bool save_tmp = false;

    while(1)
    {
        try_config.bit0.duration0 = 24; //T0H
        try_config.bit0.duration1 = 50; //T0L
        try_config.bit1.duration0 = 50; //T1H
        try_config.bit1.duration1 = 24; //T1L
        ESP_LOGW(TAGE, "\n\nNEW CYCLE\n");

        switch(master)
        {
            case 0: ESP_LOGW(TAG, LOG_COLOR_W"T0H\n"); slave = 1; break;
            case 1: ESP_LOGW(TAG, LOG_COLOR_W"T 0 L\n"); slave = 0; break;
            case 2: ESP_LOGW(TAG, LOG_COLOR_W"T 1 H\n"); slave = 0; break;
            case 3: ESP_LOGW(TAG, LOG_COLOR_W"T1L\n"); slave = 0; break;
            default: ERR_CHECK_RETURN(true);
        }

        save_tmp = true;

        while(run)
        {
            if(save_tmp)
            {
                ERR_CHECK_RETURN(rmt_bytes_encoder_update_config(led_strip_encoder.bytes_encoder, &encoder_default_config));
                memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
                ERR_CHECK_RETURN(rmt_transmit(led_chan, encoder_handle, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
                ERR_CHECK_RETURN(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(1000));

                switch(slave)
                {
                    case 0: tmp = try_config.bit0.duration0; ESP_LOGI(TAG, LOG_COLOR_I"T0H\n"); break;
                    case 1: tmp = try_config.bit0.duration1; ESP_LOGI(TAG, LOG_COLOR_I"T 0 L\n"); break;
                    case 2: tmp = try_config.bit1.duration0; ESP_LOGI(TAG, LOG_COLOR_I"T 1 H\n"); break;
                    case 3: tmp = try_config.bit1.duration1; ESP_LOGI(TAG, LOG_COLOR_I"T1L\n"); break;
                    default: ERR_CHECK_RETURN(true);
                }

                save_tmp = false;
            }

            switch(slave)
            {
                case 0: try_config.bit0.duration0--; ESP_LOGI(TAG, LOG_COLOR_W"%d"LOG_RESET_COLOR" - %d - %d - %d", try_config.bit0.duration0, try_config.bit0.duration1, try_config.bit1.duration0, try_config.bit1.duration1); break;
                case 1: try_config.bit0.duration1--; ESP_LOGI(TAG, "%d - "LOG_COLOR_W"%d"LOG_RESET_COLOR" - %d - %d", try_config.bit0.duration0, try_config.bit0.duration1, try_config.bit1.duration0, try_config.bit1.duration1); break;
                case 2: try_config.bit1.duration0--; ESP_LOGI(TAG, "%d - %d - "LOG_COLOR_W"%d"LOG_RESET_COLOR" - %d", try_config.bit0.duration0, try_config.bit0.duration1, try_config.bit1.duration0, try_config.bit1.duration1); break;
                case 3: try_config.bit1.duration1--; ESP_LOGI(TAG, "%d - %d - %d - "LOG_COLOR_W"%d", try_config.bit0.duration0, try_config.bit0.duration1, try_config.bit1.duration0, try_config.bit1.duration1); break;
                default: ERR_CHECK_RETURN(true);
            }

            ERR_CHECK_RETURN(rmt_bytes_encoder_update_config(led_strip_encoder.bytes_encoder, &try_config));

            for(uint16_t start_rgb = 0; start_rgb < 100; start_rgb++)
            {
                for(int j = 0; j < STRIP_LED_N; j++)
                {
                    hue = j * 360 / STRIP_LED_N * 2 + start_rgb;
                    value = start_rgb % 50;
                    if(value >= 25) value = 50 - value;
                    value += 2;
                    hsv2rgb(hue, 100, value, &red, &green, &blue);
                    led_strip_pixels[j * 3] = green;
                    led_strip_pixels[j * 3 + 1] = red;
                    led_strip_pixels[j * 3 + 2] = blue;
                }

                ERR_CHECK_RETURN(rmt_transmit(led_chan, encoder_handle, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
                ERR_CHECK_RETURN(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(10);
            }

            switch(slave)
            {
                case 0:
                    if(14 > try_config.bit0.duration0) {
                        try_config.bit0.duration0 = tmp;
                        save_tmp = true;
                        if(master == 1) slave = 2;
                        else slave = 1;
                    }
                    break;
                case 1:
                    if(38 > try_config.bit0.duration1) {
                        try_config.bit0.duration1 = tmp;
                        save_tmp = true;
                        if(master == 2) slave = 3;
                        else slave = 2;
                    }
                    break;
                case 2:
                    if(38 > try_config.bit1.duration0) {
                        try_config.bit1.duration0 = tmp;
                        save_tmp = true;
                        if(master == 3) {
                            if(12 < try_config.bit1.duration1) dec = true;
                            else run = false;
                            slave = 0;
                        }
                        else slave = 3;
                    }
                    break;
                case 3:
                    if(14 > try_config.bit1.duration1) {
                        try_config.bit1.duration1 = tmp;
                        save_tmp = true;
                        if(master == 0) {
                            if(12 < try_config.bit0.duration0) dec = true;
                            else run = false;
                            slave = 1;
                        }
                        else if(master == 1) {
                            if(36 < try_config.bit0.duration1) dec = true;
                            else run = false;
                            slave = 0;
                        }
                        else if(master == 2) {
                            if(36 < try_config.bit1.duration0) dec = true;
                            else run = false;
                            slave = 0;
                        }
                        else ERR_CHECK_RETURN(true);
                    }
                    break;
                default: ERR_CHECK_RETURN(true);
            }

            if(dec)
            {
                try_config.bit0.duration0--;
                try_config.bit0.duration1--;
                try_config.bit1.duration0--;
                try_config.bit1.duration1--;
                dec = false;
            }
        }

        if(master < 3) master++;
        else master = 0;
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
