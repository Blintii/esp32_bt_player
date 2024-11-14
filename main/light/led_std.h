/*
 * Standard (single pixel) LED handler
 */

#ifndef __LED_STD_H__
#define __LED_STD_H__


/* blinking mode type */
typedef enum {
    LED_STD_MODE_SLOW, // slow blinking
    LED_STD_MODE_FAST, // fast blinking
    LED_STD_MODE_DIM, // dimmed low brightness
} led_std_mode;


void led_std_init();
void led_std_set(led_std_mode mode);


#endif /* __LED_STD_H__ */
