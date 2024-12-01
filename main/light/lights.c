
#include "math.h"
#include "freertos/FreeRTOS.h"

#include "app_tools.h"
#include "lights.h"
#include "dsp.h"


typedef enum {
    FADING_TYPE_SHADER_FADE,
    FADING_TYPE_SHADER_FFT,
} fading_type;

typedef struct {
    color_hsl *color;
    size_t color_n;
    size_t pixel_n;
    fading_type type;
    union {
        struct {
            uint8_t *buf;
            lights_rgb_order offset;
        } shader_fade;
        struct {
            color_hsl *lut_pos;
        } shader_fft;
    } ctx;
} fading_ctx;


static const char *TAG = LOG_COLOR("95") "LIGHT" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "LIGHT" LOG_COLOR_E;
static lights_zone zones[LIGHTS_ZONES_SIZE];


static void lights_render_shader_color(lights_zone *zone);
static void lights_render_shader_repeat(lights_zone *zone);
static void lights_render_shader_fade(lights_zone *zone);
static void lights_render_shader_fft(lights_zone *zone);
static void fadeing(fading_ctx arg);
static void fft_fadeing(lights_shader_cfg_fft *cfg, size_t pixel_n);
static void fft_band_map(lights_zone *zone);


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
                case SHADER_SINGLE: lights_render_shader_color(zone); break;
                case SHADER_REPEAT: lights_render_shader_repeat(zone); break;
                case SHADER_FADE: lights_render_shader_fade(zone); break;
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
    ERR_CHECK_RESET(!pixels->data);
    ERR_CHECK_RESET(!pixels->pixel_n);
    ERR_CHECK_RESET(pixels->pixel_n < (pixel_offset + pixel_n));
    zone->frame_buf.data = &pixels->data[pixel_offset * 3];
    zone->frame_buf.pixel_n = pixel_n;
    zone->frame_buf.data_size = pixel_n * 3;
    zone->mled = strip;
    zone->colors = colors;

    /* set default shader */
    lights_shader *shader = &zone->shader;
    shader->type = SHADER_SINGLE;
    lights_shader_cfg_single cfg = {
        .color = {
            .r = 0,
            .g = 0,
            .b = 0
        }
    };
    shader->cfg.shader_single = cfg;
    shader->need_render = true;
    return zone;
}

void lights_shader_init_fft(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    lights_shader_cfg_fft *cfg = &zone->shader.cfg.shader_fft;
    ERR_CHECK(2 > frame_buf->pixel_n);
    ERR_IF_NULL_RETURN(cfg->colors);
    cfg->bands = (lights_shader_cfg_fft_band*)malloc(frame_buf->pixel_n * sizeof(lights_shader_cfg_fft_band));
    ERR_IF_NULL_RETURN(cfg->bands);
    cfg->pixel_lut = (color_hsl*)malloc(frame_buf->pixel_n * sizeof(color_hsl));
    ERR_IF_NULL_RETURN(cfg->pixel_lut);
    fft_fadeing(cfg, frame_buf->pixel_n);
    fft_band_map(zone);
}

static void lights_render_shader_color(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    ERR_IF_NULL_RETURN(buf);
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_single *cfg = &zone->shader.cfg.shader_single;
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

static void lights_render_shader_repeat(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    ERR_IF_NULL_RETURN(buf);
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_repeat *cfg = &zone->shader.cfg.shader_repeat;
    color_rgb *color = cfg->colors;
    ERR_IF_NULL_RETURN(color);
    size_t cur_color = 0;

    for(size_t i = 0; i < frame_buf->pixel_n; i++)
    {
        buf[offset.i_r] = color->r;
        buf[offset.i_g] = color->g;
        buf[offset.i_b] = color->b;
        buf += 3;

        if(cfg->color_n <= ++cur_color)
        {
            cur_color = 0;
            color = cfg->colors;
        }
        else color++;
    }

    zone->mled->need_update = true;
}

static void lights_render_shader_fade(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    ERR_IF_NULL_RETURN(buf);
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_fade *cfg = &zone->shader.cfg.shader_fade;
    color_hsl *color = cfg->colors;
    ERR_IF_NULL_RETURN(color);
    fading_ctx arg = {
        .type = FADING_TYPE_SHADER_FADE,
        .color = color,
        .color_n = cfg->color_n,
        .pixel_n = frame_buf->pixel_n,
        .ctx = {
            .shader_fade = {
                .buf = buf,
                .offset = offset
            }
        }
    };
    fadeing(arg);
    zone->mled->need_update = true;
}

static void lights_render_shader_fft(lights_zone *zone)
{
    mled_pixels *frame_buf = &zone->frame_buf;
    uint8_t *buf = frame_buf->data;
    ERR_IF_NULL_RETURN(buf);
    lights_rgb_order offset = zone->colors;
    lights_shader_cfg_fft *cfg = &zone->shader.cfg.shader_fft;
    ERR_IF_NULL_RETURN(cfg->bands);
    ERR_IF_NULL_RETURN(cfg->pixel_lut);
    color_hsl *color = cfg->pixel_lut;
    color_hsl mod;
    color_rgb rgb;
    float *fft_res = dsp_fft_get_res(cfg->is_right);
    lights_shader_cfg_fft_band *band = cfg->bands;
    float rms;
    float fft_val;
    float corr;

    if(fft_res)
    {
        if(cfg->mirror) buf += frame_buf->data_size - 3;

        for(size_t px_i = 0; px_i < frame_buf->pixel_n; px_i++)
        {
            rms = 0;

            for(size_t fft_i = band->fft_min; fft_i < band->fft_max; fft_i++)
            {
                fft_val = fft_res[fft_i];
                /* apply higher frequency usually lower values correction
                * correction mul range [1..100] */
                corr = (fft_i * 99.0f);
                corr /= (float)DSP_FFT_RES_N - 1.0f;
                fft_val *= 1.0f + corr;
                rms += powf(fft_val, 2.0f);
            }

            rms = sqrtf(rms / (float)band->fft_width);
            /* apply LED brightness rough gamma correction */
            rms *= rms;
            mod = *color;
            mod.lum *= rms * cfg->intensity;

            if(mod.lum > color->lum) mod.lum = color->lum;

            rgb = color_hsl_to_rgb(mod);
            buf[offset.i_r] = rgb.r;
            buf[offset.i_g] = rgb.g;
            buf[offset.i_b] = rgb.b;

            if(cfg->mirror) buf -= 3;
            else buf += 3;

            color++;
            band++;
        }
    }
    else
    {
        for(size_t px_i = 0; px_i < frame_buf->pixel_n; px_i++)
        {
            buf[offset.i_r] = 0;
            buf[offset.i_g] = 0;
            buf[offset.i_b] = 0;
            buf += 3;
        }

    }

    zone->mled->need_update = true;
    /* to keep fft rendering live */
    zone->shader.need_render = true;
}

static void fadeing(fading_ctx arg)
{
    size_t color_left = arg.color_n;
    size_t pixel_left = arg.pixel_n;
    color_hsl *color = arg.color;
    color_hsl work;
    color_hsl step;
    color_hsl next;
    size_t section_left;
    float hue_nearest, hue_test;

    while(color_left)
    {
        work = *color;
        step = (color_hsl) {0, 0, 0};

        if(1 < color_left)
        {
            next = *(color + 1);
            section_left = (pixel_left - 1) / (color_left - 1);

            if(section_left)
            {
                /* find nearest hue instead of go through the wrong way in the color wheel */
                hue_nearest = next.hue - work.hue;
                hue_test = hue_nearest + 360;

                if(fabsf(hue_test) < fabsf(hue_nearest)) hue_nearest = hue_test;
                else
                {
                    hue_test = hue_nearest - 360;

                    if(fabsf(hue_test) < fabsf(hue_nearest)) hue_nearest = hue_test;
                }

                step.hue = hue_nearest / (float)section_left;
                step.sat = (next.sat - work.sat) / (float)section_left;
                step.lum = (next.lum - work.lum) / (float)section_left;
            }
        }
        else section_left = pixel_left;

        pixel_left -= section_left;

        while(section_left)
        {
            switch(arg.type)
            {
                case FADING_TYPE_SHADER_FADE: {
                    color_rgb rgb = color_hsl_to_rgb(work);
                    lights_rgb_order offset = arg.ctx.shader_fade.offset;
                    uint8_t *buf = arg.ctx.shader_fade.buf;
                    buf[offset.i_r] = rgb.r;
                    buf[offset.i_g] = rgb.g;
                    buf[offset.i_b] = rgb.b;
                    arg.ctx.shader_fade.buf += 3;
                    break;
                }
                case FADING_TYPE_SHADER_FFT: {
                    *arg.ctx.shader_fft.lut_pos++ = work;
                    break;
                }
                default:
                    ERR_BAD_CASE(arg.type, "%d");
                    break;
            }

            work.hue += step.hue;
            work.sat += step.sat;
            work.lum += step.lum;
            section_left--;
        }

        color_left--;
        color++;
    }
}

static void fft_fadeing(lights_shader_cfg_fft *cfg, size_t pixel_n)
{
    fading_ctx arg = {
        .type = FADING_TYPE_SHADER_FFT,
        .color = cfg->colors,
        .color_n = cfg->color_n,
        .pixel_n = pixel_n,
        .ctx = {
            .shader_fft = {
                .lut_pos = cfg->pixel_lut
            }
        }
    };
    fadeing(arg);
}

static void fft_band_map(lights_zone *zone)
{
    lights_shader_cfg_fft *cfg = &zone->shader.cfg.shader_fft;
    lights_shader_cfg_fft_band *bands = cfg->bands;
    size_t pixel_n = zone->frame_buf.pixel_n;
    float fft_n = DSP_FFT_RES_N - 1.0f;
    float mult = fft_n - pixel_n;
    float i_max = pixel_n - 1.0f;
    /* start from 1. value (0. value is signal's DC component) */
    size_t cur_max = 1;
    size_t last_max = 1;
    size_t sum = 0;
    size_t width;

    for(size_t i = 0; i < pixel_n; i++)
    {
        bands->fft_min = last_max;

        if(pixel_n - i - 1)
        {
            /* apply logaritmic stepping */
            cur_max = mult * powf((float)i / i_max, M_E);
            /* apply linear stepping */
            cur_max += i;

            /* correct boundaries to guarantee at least 1 band mapping 1 pixel */
            if(cur_max > last_max) width = cur_max - last_max;
            else
            {
                cur_max = last_max + 1;
                width = 1;
            }

            last_max = cur_max;
            bands->fft_max = cur_max;
        }
        else
        {
            /* last item corrected with actual step summary */
            width = fft_n - sum;
            bands->fft_max = fft_n;
        }

        bands->fft_width = width;

        // switch(i%3)
        // {
        //     case 2:
        //         ESP_LOGI(TAG, "[%d...%d]: %d", bands->fft_min, bands->fft_max, bands->fft_width);
        //         break;
        //     case 1:
        //         ESP_LOGI(TAG, LOG_COLOR_W"[%d...%d]: %d", bands->fft_min, bands->fft_max, bands->fft_width);
        //         break;
        //     default:
        //         ESP_LOGI(TAG, LOG_COLOR_I"[%d...%d]: %d", bands->fft_min, bands->fft_max, bands->fft_width);
        //         break;

        // }

        sum += width;
        bands++;
    }

    // ESP_LOGW(TAG, LOG_COLOR_E"SUM: %d", sum);
}
