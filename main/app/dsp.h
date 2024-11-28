/*
 * DSP (Digital Signal Processing)
 */

#ifndef __APP_DSP_H__
#define __APP_DSP_H__


#include <stdbool.h>


#define DSP_FFT_IN_N 1024
#define DSP_CHANNEL 2
#define DSP_FFT_RES_N (DSP_FFT_IN_N / 2)
#define DSP_FFT_BUF_N (DSP_FFT_IN_N * DSP_CHANNEL)


/* values copied to "out" array, values range: [0..1] */
void dsp_fft_get_res(float *out, bool is_right);


#endif /* __APP_DSP_H__ */
