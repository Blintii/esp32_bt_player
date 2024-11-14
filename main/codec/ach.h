/*
 * ACH (Audio Codec Handler)
 */

#ifndef __AUDIO_CODEC_HANDLER_H__
#define __AUDIO_CODEC_HANDLER_H__

#include <stdint.h>
#include <stddef.h>


void ach_control_init();
void ach_set_volume(uint8_t vol);
void ach_unmute();
void ach_mute();

void ach_I2S_start();
void ach_I2S_stop();
void ach_I2S_enable_channel();
void ach_I2S_disable_channel();
void ach_I2S_write(const void *src, size_t size);


#endif /* __AUDIO_CODEC_HANDLER_H__ */
