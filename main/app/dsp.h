/*
 * DSP (Digital Signal Processing)
 */

#ifndef __APP_DSP_H__
#define __APP_DSP_H__


#include <stdbool.h>


#define DSP_CHANNEL 2
#define DSP_DATA_LEN sizeof(uint16_t) // 16 bit (2 byte)
#define DSP_FFT_EXP 11
#define DSP_FFT_IN_N (1 << DSP_FFT_EXP) // x power of 2
#define DSP_FFT_RES_N (DSP_FFT_IN_N / 2)
#define DSP_FFT_BUF_N (DSP_FFT_IN_N * DSP_CHANNEL * DSP_DATA_LEN)


typedef struct {
	float re;
	float im;
} dsp_comp;


void dsp_init();
void dsp_fft_do();
void dsp_fft_finalize();
void dsp_work_buf_init();
void dsp_fft(dsp_comp* in_R, dsp_comp* in_L);
void dsp_reverse_bits(dsp_comp* in_R, dsp_comp* in_L, float* out_R, float* out_L);
/* values range: [0..1] */
float *dsp_fft_get_res(bool is_right);
void dsp_new_data(const uint8_t *data, size_t size);


#endif /* __APP_DSP_H__ */
