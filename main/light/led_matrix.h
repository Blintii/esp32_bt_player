/*
 * Matrix (multiple pixel, addressable) LED strip handler
 */

#ifndef __LED_MATRIX_H__
#define __LED_MATRIX_H__


#include "stdint.h"
#include "stddef.h"
#include "driver/rmt_encoder.h"

#include "app_config.h"


#define MLED_CLOCK_HZ 80000000
#define MLED_US_TO_DURATION(us) ((MLED_CLOCK_HZ/1000000.0f)*us)
#define MLED_NS_TO_DURATION(ns) ((MLED_CLOCK_HZ/1000000000.0f)*ns)


typedef struct {
    uint8_t *data;
    size_t data_size;
    size_t pixel_n;
} mled_pixels;

typedef struct {
    uint8_t i_r;
    uint8_t i_g;
    uint8_t i_b;
} mled_rgb_order;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *payload_handler;
    rmt_encoder_t *eof_handler;
    rmt_channel_handle_t tx_channel;
    const rmt_symbol_word_t *reset_code;
    size_t reset_code_size;
    mled_pixels pixels;
    mled_rgb_order rgb_order;
    bool data_sent;
} mled_strip;


extern mled_strip mled_channels[MLED_STRIP_N];


void mled_init();
void mled_encode_chain_ws281x(mled_strip *strip);
void mled_set_size(mled_strip *strip, size_t pixel_n);
void mled_update(mled_strip *strip);


#endif /* __LED_MATRIX_H__ */
