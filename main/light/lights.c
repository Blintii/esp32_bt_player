
#include "freertos/FreeRTOS.h"

#include "app_tools.h"
#include "lights.h"


#define LIGHTS_ZONES_SIZE 16


static const char *TAG = LOG_COLOR("95") "LIGHT" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "LIGHT" LOG_COLOR_E;
static lights_zone zones[LIGHTS_ZONES_SIZE];


void lights_main()
{
    mled_strip *strip;
    lights_zone *zone;
    bool strip_will_update;

    for(size_t strip_index = 0; strip_index < MLED_CHANNEL_N; strip_index++)
    {
        strip = &mled_channels[strip_index];
        strip_will_update = false;

        /* search zone used current strip, and reset need_update flags */
        for(size_t zone_index = 0; zone_index < MLED_CHANNEL_N; zone_index++)
        {
            zone = &zones[zone_index];

            if(zone->live && zone->need_update)
            {
                strip_will_update = true;
                zone->need_update = false;
            }
        }

        if(strip_will_update) mled_update(strip);
    }
}

void lights_set_strip_size(size_t strip_index, size_t pixel_n)
{
    ERR_CHECK_RESET(MLED_CHANNEL_N <= strip_index);
    mled_strip *strip = &mled_channels[strip_index];

    if(strip->pixels.pixel_n > pixel_n)
    {
        for(size_t i = 0; i < LIGHTS_ZONES_SIZE; i++)
        {
            ERR_CHECK_RESET(zones[i].live && (zones[i].mled == strip));
        }
    }

    mled_set_size(strip, pixel_n);
}

void lights_set_zone(size_t zone_index, size_t strip_index, size_t pixel_offset, size_t pixel_n, lights_rgb_order colors)
{
    ERR_CHECK_RESET(LIGHTS_ZONES_SIZE <= zone_index);
    ERR_CHECK_RESET(MLED_CHANNEL_N <= strip_index);
    lights_zone *zone = &zones[zone_index];
    mled_strip *strip = &mled_channels[strip_index];
    mled_pixels *pixels = &strip->pixels;
    ERR_CHECK_RESET(!pixels->data || !pixels->pixel_n);
    ERR_CHECK_RESET(pixels->pixel_n < (pixel_offset + pixel_n));
    zone->frame_buf.data = &pixels->data[pixel_offset * 3];
    zone->frame_buf.pixel_n = pixel_n;
    zone->frame_buf.data_size = pixel_n * 3;
    zone->colors = colors;
    zone->live = true;
}

// void lights_test()
// {
//     mled_strip *strip;
//     mled_pixels *pixels;

//     for(size_t i = 0; i < MLED_CHANNEL_N; i++)
//     {
//         strip = &mled_channels[i];
//         mled_update(strip);
//         ESP_LOGI(TAG, "clear LED strip %d OK", i);
//     }

//     vTaskDelay(10);

//     strip = &mled_channels[0];
//     pixels = &strip->pixels;

//     for(int j = 0; j < STRIP_0_LED_N; j++)
//     {
//         switch(j%4)
//         {
//             case 0:
//                 pixels->data[j * 3] = 60;
//                 pixels->data[j * 3 + 1] = 0;
//                 pixels->data[j * 3 + 2] = 0;
//                 break;
//             case 1:
//                 pixels->data[j * 3] = 20;
//                 pixels->data[j * 3 + 1] = 18;
//                 pixels->data[j * 3 + 2] = 6;
//                 break;
//             case 2:
//                 pixels->data[j * 3] = 0;
//                 pixels->data[j * 3 + 1] = 40;
//                 pixels->data[j * 3 + 2] = 0;
//                 break;
//             case 3:
//                 pixels->data[j * 3] = 20;
//                 pixels->data[j * 3 + 1] = 18;
//                 pixels->data[j * 3 + 2] = 6;
//                 break;
//         }
//     }
//     mled_update(strip);

//     strip = &mled_channels[1];
//     pixels = &strip->pixels;

//     for(int j = 0; j < STRIP_1_LED_N; j++)
//     {
//         pixels->data[j * 3] = 8;
//         pixels->data[j * 3 + 1] = 0;
//         pixels->data[j * 3 + 2] = 4;
//     }
//     mled_update(strip);
// }
