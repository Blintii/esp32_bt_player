
#include "driver/ledc.h"

#include "app_tools.h"
#include "app_config.h"
#include "led_std.h"


static const char *TAG = LOG_COLOR("96") "SLED" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "SLED" LOG_COLOR_E;


static void led_std_cfg(uint32_t clk_div, uint32_t duty);


void sled_init()
{
    ESP_LOGI(TAG, "init...");
    ledc_timer_config_t ledc_cfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 100,
        .clk_cfg = LEDC_REF_TICK
    };
    ERR_CHECK_RESET(ledc_timer_config(&ledc_cfg));

    ledc_channel_config_t ledc_ch_cfg = {
        .gpio_num = PIN_LED_BLUE,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ERR_CHECK_RESET(ledc_channel_config(&ledc_ch_cfg));
    ESP_LOGI(TAG, "init OK");
}

void sled_set(led_std_mode mode)
{
    switch(mode)
    {
        case SLED_MODE_SLOW:
            led_std_cfg(66000, 900);
            ESP_LOGI(TAG, "blue LED mode set: SLOW");
            break;
        case SLED_MODE_FAST:
            led_std_cfg(12000, 1600);
            ESP_LOGI(TAG, "blue LED mode set: FAST");
            break;
        case SLED_MODE_DIM:
            led_std_cfg(256, 200);
            ESP_LOGI(TAG, "blue LED mode set: DIM");
            break;
        default:
            ERR_BAD_CASE(mode, "%d");
            break;
    }
    list_tasks_stack_info();
}

/* 3906 div, 16bit: 1Hz
 * 62500 div, 12bit: 1Hz
 * 625 div, 12bit: 100Hz */
static void led_std_cfg(uint32_t clk_div, uint32_t duty)
{
    /* LED proper freq and duty change required process */
    ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, 0);
    ledc_timer_pause(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    ledc_timer_set(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, clk_div, LEDC_TIMER_12_BIT, LEDC_REF_TICK);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_timer_resume(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    ledc_timer_rst(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
}
