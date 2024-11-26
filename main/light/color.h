/*
 * Color utilities
 */

#ifndef __COLOR_H__
#define __COLOR_H__


#include <stdint.h>


typedef struct {
    float hue; // [0..360] -> [red...green...blue...red] (120° green, 240° blue)
    float sat; // [0..1] -> [grayscale...saturated]
    float lum; // [0..1] -> [black...white] (0.5 allow max color saturation)
} color_hsl;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_rgb;


color_rgb color_hsl_to_rgb(color_hsl hsl);


#endif /* __COLOR_H__ */
