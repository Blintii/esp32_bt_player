/*
 * Lights handler for calculate LED strips pixel data
 */

#ifndef __LIGHTS_H__
#define __LIGHTS_H__


#include <stdint.h>

#include "led_matrix.h"
#include "color.h"


typedef struct {
    uint8_t i_r;
    uint8_t i_g;
    uint8_t i_b;
} lights_rgb_order;

typedef enum {
    SHADER_SINGLE,
    SHADER_REPEAT,
    SHADER_FADE,
    SHADER_FFT
} lights_shader_type;

typedef struct {
    color_rgb color;
} lights_shader_cfg_single;

typedef struct {
    color_rgb *colors;
    size_t color_n;
} lights_shader_cfg_repeat;

typedef struct {
    color_hsl *colors;
    size_t color_n;
} lights_shader_cfg_fade;

typedef struct {
    size_t fft_min;
    size_t fft_max;
    size_t fft_width;
} lights_shader_cfg_fft_band;

typedef struct {
    color_hsl *colors;
    size_t color_n;
    bool is_right; // audio channel
    color_hsl *pixel_lut; // store each pixel color
    lights_shader_cfg_fft_band *bands;
    float intensity;
    bool mirror;
} lights_shader_cfg_fft;

typedef struct {
    lights_shader_type type;
    bool need_render;
    /* garanteed with union to largest cfg struct size can fit */
    union {
        lights_shader_cfg_single shader_single;
        lights_shader_cfg_repeat shader_repeat;
        lights_shader_cfg_fade shader_fade;
        lights_shader_cfg_fft shader_fft;
    } cfg;
} lights_shader;

typedef struct {
    mled_strip *mled;
    mled_pixels frame_buf;
    lights_rgb_order colors;
    lights_shader shader;
} lights_zone;


void lights_main();
void lights_set_strip_size(size_t strip_index, size_t pixel_n);
lights_zone *lights_set_zone(size_t zone_index, size_t strip_index, size_t pixel_offset, size_t pixel_n, lights_rgb_order colors);
void lights_shader_init_fft(lights_zone *zone);


#endif /* __LIGHTS__ */
