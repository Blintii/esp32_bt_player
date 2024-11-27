
#include "math.h"

#include "color.h"


static float color_fn(color_hsl hsl, float a, float n);

/* implementation of:
 * https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative */
color_rgb color_hsl_to_rgb(color_hsl hsl)
{
    float a = hsl.sat * fminf(hsl.lum, 1 - hsl.lum);
    color_rgb rgb = {
        .r = roundf(255.0f * color_fn(hsl, a, 0.0f)),
        .g = roundf(255.0f * color_fn(hsl, a, 8.0f)),
        .b = roundf(255.0f * color_fn(hsl, a, 4.0f))
    };
    return rgb;
}

static float color_fn(color_hsl hsl, float a, float n)
{
    float k = fmodf((n + (hsl.hue / 30.0f)), 12.0f);
    return hsl.lum - (a * fmaxf(-1.0f, fminf((k - 3.0f), fminf((9 - k), 1.0f))));
}
