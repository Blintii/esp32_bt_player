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
    SHADER_COLOR,
    SHADER_COLORS_REPEAT,
    SHADER_COLORS_FADE,
    SHADER_FFT
} lights_shader_type;

typedef struct {
    color_rgb color;
} lights_shader_cfg_color;

typedef struct {
    color_rgb *colors;
    size_t color_n;
} lights_shader_cfg_colors_repeat;

typedef struct {
    color_hsl *colors;
    size_t color_n;
} lights_shader_cfg_colors_fade;

typedef struct {
    color_hsl *colors;
    size_t color_n;
    bool is_right;
} lights_shader_cfg_fft;

typedef struct {
    lights_shader_type type;
    bool need_render;
    /* garanteed with union to largest cfg struct size can fit */
    union {
        lights_shader_cfg_color shader_color;
        lights_shader_cfg_colors_repeat shader_colors_repeat;
        lights_shader_cfg_colors_fade shader_colors_fade;
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


#endif /* __LIGHTS__ */
