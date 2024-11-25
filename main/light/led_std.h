/*
 * Standard (single pixel) LED handler
 */

#ifndef __LED_STD_H__
#define __LED_STD_H__


/* blinking mode type */
typedef enum {
    SLED_MODE_SLOW, // slow blinking
    SLED_MODE_FAST, // fast blinking
    SLED_MODE_DIM, // dimmed low brightness
} led_std_mode;


void sled_init();
void sled_set(led_std_mode mode);


#endif /* __LED_STD_H__ */
