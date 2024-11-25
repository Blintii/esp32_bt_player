/*
 * Lights handler for calculate LED strips pixel data
 */

#ifndef __LIGHTS_H__
#define __LIGHTS_H__


#include <stdint.h>

#include "led_matrix.h"


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} lights_rgb_order;

typedef struct {
    mled_pixels *frame_buf;
    mled_strip *mled;
    lights_rgb_order colors;
    bool need_update;
} lights_strip;


void lights_test();


#endif /* __LIGHTS__ */
