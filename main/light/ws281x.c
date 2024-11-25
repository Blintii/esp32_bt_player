
#include "app_tools.h"
#include "led_matrix.h"


static const char *TAGE = "WS281x";
/* from WS281x cascade RGB LED chip datasheet:
 *
 * 0 code:
 *   __________________
 *  |                  |       T0L: 580ns~1.6µs       |
 *  | T0H: 220ns~380ns |______________________________|
 *
 * 1 code:
 *                                  __________________
 *  |       T1H: 580ns~1.6µs       |                  |
 *  |______________________________| T1L: 220ns~420ns |
 */
static const rmt_bytes_encoder_config_t ws281x_encoder_cfg = {
    .bit0 = { // 0 code, if bit is set
        .level0 = 1,
        .duration0 = MLED_NS_TO_DURATION(240), // T0H
        .level1 = 0,
        .duration1 = MLED_NS_TO_DURATION(600), // T0L
    },
    .bit1 = { // 1 code, if bit is not set
        .level0 = 1,
        .duration0 = MLED_NS_TO_DURATION(600), // T1H
        .level1 = 0,
        .duration1 = MLED_NS_TO_DURATION(240), // T1L
    },
    .flags.msb_first = 1 // WS281x require high bit data is first: 7 -> 0
};
static const rmt_symbol_word_t ws281x_reset_code = {
    .level0 = 0,
    .duration0 = MLED_US_TO_DURATION(300), // reset time in WS281x need >280µs
    .level1 = 0,
    .duration1 = 1 // the zero duration used only internally as TX EOF flag by RMT
};


void mled_encode_chain_ws281x(mled_strip *strip)
{
    /* byte encoder used to transform 1 bit to "0 code" and "1 code" for WS281x */
    ERR_CHECK_RESET(rmt_new_bytes_encoder(&ws281x_encoder_cfg, &strip->payload_handler));

    /* copy encoder used for send WS281x "reset code" (time, while low level needed) */
    rmt_copy_encoder_config_t copy_encoder_cfg;
    ERR_CHECK_RESET(rmt_new_copy_encoder(&copy_encoder_cfg, &strip->eof_handler));
    /* copy encoder needs constant data what to send */
    strip->reset_code = &ws281x_reset_code;
    strip->reset_code_size = sizeof(ws281x_reset_code);
}
