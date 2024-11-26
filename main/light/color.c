
#include "math.h"

#include "color.h"


static float color_fn(color_hsl hsl, float a, float n);

/* implementation of:
 * https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative */
color_rgb color_hsl_to_rgb(color_hsl hsl)
{
    float a = hsl.sat * fminf(hsl.lum, 1 - hsl.lum);
    color_rgb rgb = {
        .r = roundf(255 * color_fn(hsl, a, 0)),
        .g = roundf(255 * color_fn(hsl, a, 8)),
        .b = roundf(255 * color_fn(hsl, a, 4))
    };
    return rgb;
}

static float color_fn(color_hsl hsl, float a, float n)
{
    float k = fmodf((n + (hsl.hue / 30.0)), 12.0);
    return hsl.lum - (a * fmaxf(-1, fminf((k - 3), fminf((9 - k), 1))));
}
