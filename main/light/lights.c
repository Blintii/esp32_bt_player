
#include "freertos/FreeRTOS.h"

#include "app_tools.h"
#include "lights.h"


#define STRIP_0_LED_N 50
#define STRIP_1_LED_N 266


static const char *TAG = LOG_COLOR("95") "LIGHT" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "LIGHT" LOG_COLOR_E;
static lights_strip strips[MLED_CHANNEL_N];


void lights_test()
{
    mled_strip *strip;
    mled_pixels *pixels;
    mled_set_size(&mled_channels[0], STRIP_0_LED_N);
    mled_set_size(&mled_channels[1], STRIP_1_LED_N);

    for(uint8_t i = 0; i < MLED_CHANNEL_N; i++)
    {
        strip = &mled_channels[i];
        mled_update(strip);
        ESP_LOGI(TAG, "clear LED strip %d OK", i);
    }

    vTaskDelay(10);

    strip = &mled_channels[0];
    pixels = &strip->pixels;

    for(int j = 0; j < STRIP_0_LED_N; j++)
    {
        switch(j%4)
        {
            case 0:
                pixels->data[j * 3] = 60;
                pixels->data[j * 3 + 1] = 0;
                pixels->data[j * 3 + 2] = 0;
                break;
            case 1:
                pixels->data[j * 3] = 20;
                pixels->data[j * 3 + 1] = 18;
                pixels->data[j * 3 + 2] = 6;
                break;
            case 2:
                pixels->data[j * 3] = 0;
                pixels->data[j * 3 + 1] = 40;
                pixels->data[j * 3 + 2] = 0;
                break;
            case 3:
                pixels->data[j * 3] = 20;
                pixels->data[j * 3 + 1] = 18;
                pixels->data[j * 3 + 2] = 6;
                break;
        }
    }
    mled_update(strip);

    strip = &mled_channels[1];
    pixels = &strip->pixels;

    for(int j = 0; j < STRIP_1_LED_N; j++)
    {
        pixels->data[j * 3] = 8;
        pixels->data[j * 3 + 1] = 0;
        pixels->data[j * 3 + 2] = 4;
    }
    mled_update(strip);
}
