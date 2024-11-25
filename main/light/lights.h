/*
 * Lights handler for calculate LED strips pixel data
 */

#ifndef __LIGHTS_H__
#define __LIGHTS_H__


#include <stdint.h>

#include "led_matrix.h"


typedef struct {
    uint8_t i_r;
    uint8_t i_g;
    uint8_t i_b;
} lights_rgb_order;

typedef struct {
    mled_strip *mled;
    mled_pixels frame_buf;
    lights_rgb_order colors;
    bool live;
    bool need_update;
} lights_zone;


void lights_main();
void lights_set_strip_size(size_t strip_index, size_t pixel_n);
void lights_set_zone(size_t zone_index, size_t strip_index, size_t pixel_offset, size_t pixel_n, lights_rgb_order colors);


#endif /* __LIGHTS__ */
