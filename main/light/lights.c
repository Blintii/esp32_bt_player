
#include "freertos/FreeRTOS.h"

#include "app_tools.h"
#include "lights.h"


#define LIGHTS_ZONES_SIZE 16


static const char *TAG = LOG_COLOR("95") "LIGHT" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "LIGHT" LOG_COLOR_E;
static lights_zone zones[LIGHTS_ZONES_SIZE];


static void lights_render_shader_color(lights_zone *zone);
static void lights_render_shader_colors_repeat(lights_zone *zone);
static void lights_render_shader_fft(lights_zone *zone);


void lights_main()
{
    mled_strip *strip;
    lights_zone *zone;

    for(size_t zone_index = 0; zone_index < LIGHTS_ZONES_SIZE; zone_index++)
    {
        zone = &zones[zone_index];

        if(zone->mled && zone->shader.need_render)
        {
            zone->shader.need_render = false;

            switch(zone->shader.type)
            {
                case SHADER_COLOR: lights_render_shader_color(zone); break;
                case SHADER_COLORS_REPEAT: lights_render_shader_colors_repeat(zone); break;
                case SHADER_FFT: lights_render_shader_fft(zone); break;
                default: PRINT_TRACE(); break;
            }
        }
    }

    for(size_t strip_index = 0; strip_index < MLED_CHANNEL_N; strip_index++)
    {
        strip = &mled_channels[strip_index];

        if(strip->need_update)
        {
            strip->need_update = false;
            mled_update(strip);
        }
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
            ERR_CHECK_RESET(zones[i].mled == strip);
        }
    }

    mled_set_size(strip, pixel_n);
}

lights_zone *lights_set_zone(size_t zone_index, size_t strip_index, size_t pixel_offset, size_t pixel_n, lights_rgb_order colors)
{
    ESP_LOGI(TAG, "set zone %d, in strip %d", zone_index, strip_index);
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
    zone->mled = strip;
    zone->colors = colors;

    /* set default shader */
    lights_shader *shader = &zone->shader;
    shader->type = SHADER_COLOR;
    lights_shader_cfg_color cfg = {
        .color = {
            .r = 0,
            .g = 0,
            .b = 0
        }
    };
    shader->cfg.shader_color = cfg;
    shader->need_render = true;
    return zone;
}

static void lights_render_shader_color(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_color *cfg = &zone->shader.cfg.shader_color;
    color_rgb color = cfg->color;

    for(size_t i = 0; i < frame_buf->pixel_n; i++)
    {
        buf[offset.i_r] = color.r;
        buf[offset.i_g] = color.g;
        buf[offset.i_b] = color.b;
        buf += 3;
    }

    zone->mled->need_update = true;
}

static void lights_render_shader_colors_repeat(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_colors_repeat *cfg = &zone->shader.cfg.shader_colors_repeat;
    uint8_t *color = cfg->colors;
    size_t cur_color = 0;

    for(size_t i = 0; i < frame_buf->pixel_n; i++)
    {
        buf[offset.i_r] = *color++;
        buf[offset.i_g] = *color++;
        buf[offset.i_b] = *color++;
        buf += 3;

        if(cfg->color_n <= ++cur_color)
        {
            cur_color = 0;
            color = cfg->colors;
        }
    }

    zone->mled->need_update = true;
}

static void lights_render_shader_fft(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    lights_rgb_order offset = zone->colors;
    // lights_shader_cfg_fft *cfg = &zone->shader.cfg.shader_fft;
    uint8_t r = 0, g = 0, b = 0;

    for(size_t i = 0; i < frame_buf->pixel_n; i++)
    {
        buf[offset.i_r] = r;
        buf[offset.i_g] = g;
        buf[offset.i_b] = b;
        buf += 3;
    }

    zone->mled->need_update = true;
}
