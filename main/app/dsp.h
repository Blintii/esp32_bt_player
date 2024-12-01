/*
 * DSP (Digital Signal Processing)
 */

#ifndef __APP_DSP_H__
#define __APP_DSP_H__


#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

#include "app_config.h"


#define DSP_FFT_EXP 11
#define DSP_FFT_IN_N (1 << DSP_FFT_EXP) // x power of 2
#define DSP_FFT_RES_N (DSP_FFT_IN_N / 2)
#define DSP_FFT_BUF_N (DSP_FFT_IN_N * AUDIO_CHANNEL_N * AUDIO_SAMPLE_BYTE_LEN)


typedef struct {
	float re;
	float im;
} dsp_comp;


extern const float window_lut[DSP_FFT_IN_N];
extern const dsp_comp twiddle_lut[DSP_FFT_RES_N];
extern const uint16_t rev_bits_lut[DSP_FFT_IN_N];


bool dsp_fft_buf_create();
void dsp_fft_buf_del();
void dsp_fft_do();
void dsp_fft_finalize();
void dsp_work_buf_init();
void dsp_fft(dsp_comp* in_R, dsp_comp* in_L);
void dsp_reverse_bits(dsp_comp* in_R, dsp_comp* in_L, float* out_R, float* out_L);
/* values range: [0..1] */
float *dsp_fft_get_res(bool is_right);
void dsp_new_data(const uint8_t *data, size_t size);


#endif /* __APP_DSP_H__ */
