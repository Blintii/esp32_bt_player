/*
 * Stereo audio codec controller handler
 *  WM8960 codec implemented
 */

#ifndef __STEREO_CODEC_H__
#define __STEREO_CODEC_H__

#include <stdint.h>
#include <stddef.h>


#define LOG_STEREO_CODEC "CODEC"


void stereo_codec_control_init();
void stereo_codec_set_volume(uint8_t vol);

void stereo_codec_I2S_start();
void stereo_codec_I2S_stop();
void stereo_codec_I2S_write(const void *src, size_t size, uint32_t timeout_ms);


extern const uint8_t lut_out1vol[128];
extern const uint8_t lut_dacvol[128];


#endif /* __STEREO_CODEC_H__ */
