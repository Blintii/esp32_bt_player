/*
 * Lights handler for calculate LED strips pixel data
 */

#ifndef __LIGHTS_H__
#define __LIGHTS_H__


#include <stdint.h>

#include "led_matrix.h"
#include "color.h"


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
    lights_shader_cfg_fft_band *bands;
    bool is_right; // audio channel
    color_hsl *pixel_lut; // store each pixel color
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
    lights_shader shader;
    void *next;
} lights_zone_chain;

typedef struct {
    lights_zone_chain *first;
    lights_zone_chain *last;
    size_t zone_n;
    size_t pixel_used_pos;
} lights_zone_list;


extern lights_zone_list lights_zones[MLED_STRIP_N];


void lights_main();
void lights_set_strip_size(size_t strip_index, size_t pixel_n);
void lights_new_zone(size_t strip_index, size_t pixel_n);
void lights_shader_init_fft(lights_zone_chain *zone);


#endif /* __LIGHTS__ */
