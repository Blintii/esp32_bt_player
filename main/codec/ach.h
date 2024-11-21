/*
 * ACH (Audio Codec Handler)
 */

#ifndef __AUDIO_CODEC_HANDLER_H__
#define __AUDIO_CODEC_HANDLER_H__

#include <stdint.h>
#include <stddef.h>


void ach_control_init();
void ach_volume(uint8_t vol);
void ach_unmute();
void ach_mute();

void ach_player_init(uint32_t *total_dma_buf_size);
void ach_player_start();
void ach_player_stop();
void ach_player_data(const void *src, size_t size);


#endif /* __AUDIO_CODEC_HANDLER_H__ */
