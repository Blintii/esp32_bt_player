
#include "math.h"
#include "string.h"
#include "esp_heap_caps.h"

#include "app_tools.h"
#include "dsp.h"


static const char *TAG = LOG_COLOR("37") "DSP";
static const char *TAGE = LOG_COLOR("37") "DSP" LOG_COLOR_E;
static dsp_comp *fft_work_r = NULL;
static dsp_comp *fft_work_l = NULL;
static float *fft_res_r = NULL;
static float *fft_res_l = NULL;

/* ringbuf contains DSP_DATA_LEN width signed values */
static uint8_t *ringbuf = NULL;
/* ringbuf_i means the oldest data index,
 * which pointed the data that can override first,
 * and than become latest data */
static size_t ringbuf_i = 0;


static void decode_ringbuf_val(size_t *buf_i, dsp_comp *dest, float window_val);
static void butterfly(dsp_comp* A, dsp_comp* B, const dsp_comp* w);
static void butterfly_w0(dsp_comp* A, dsp_comp* B);
static void normalize_output(dsp_comp* in, float* out);


bool dsp_fft_buf_create()
{
    ESP_LOGI(TAG, "fft buf create...");
    if(!fft_work_r) fft_work_r = (dsp_comp*)heap_caps_calloc(DSP_FFT_IN_N, sizeof(dsp_comp), MALLOC_CAP_32BIT);
    ERR_IF_NULL_RETURN_VAL(fft_work_r, false);
    if(!fft_work_l) fft_work_l = (dsp_comp*)heap_caps_calloc(DSP_FFT_IN_N, sizeof(dsp_comp), MALLOC_CAP_32BIT);
    ERR_IF_NULL_RETURN_VAL(fft_work_l, false);
    if(!fft_res_r) fft_res_r = (float*)heap_caps_calloc(DSP_FFT_RES_N, sizeof(float), MALLOC_CAP_32BIT);
    ERR_IF_NULL_RETURN_VAL(fft_res_r, false);
    if(!fft_res_l) fft_res_l = (float*)heap_caps_calloc(DSP_FFT_RES_N, sizeof(float), MALLOC_CAP_32BIT);
    ERR_IF_NULL_RETURN_VAL(fft_res_l, false);
    if(!ringbuf) ringbuf = (uint8_t*)heap_caps_calloc(DSP_FFT_BUF_N, 1, MALLOC_CAP_32BIT);
    ERR_IF_NULL_RETURN_VAL(ringbuf, false);
    ESP_LOGI(TAG, "fft buf create OK");
    return true;
}

void dsp_fft_buf_del()
{
    ESP_LOGI(TAG, "fft buf delete");
    heap_caps_free(fft_work_r);
    fft_work_r = NULL;
    heap_caps_free(fft_work_l);
    fft_work_l = NULL;
    heap_caps_free(fft_res_r);
    fft_res_r = NULL;
    heap_caps_free(fft_res_l);
    fft_res_l = NULL;
    heap_caps_free(ringbuf);
    ringbuf = NULL;
}

void dsp_fft_do()
{
    dsp_fft(fft_work_r, fft_work_l);
}

void dsp_fft_finalize()
{
    dsp_reverse_bits(fft_work_r, fft_work_l, fft_res_r, fft_res_l);
}

void dsp_work_buf_init()
{
    size_t buf_i = ringbuf_i;
    dsp_comp *R_p = fft_work_r;
	dsp_comp *L_p = fft_work_l;
	const float *w_p = window_lut;

    for(size_t data_i = 0; data_i < DSP_FFT_IN_N; data_i++)
    {
        decode_ringbuf_val(&buf_i, L_p++, *w_p);
        decode_ringbuf_val(&buf_i, R_p++, *w_p++);
    }
}

void dsp_fft(dsp_comp* in_R, dsp_comp* in_L)
{
    size_t cur_N = DSP_FFT_IN_N;
    size_t w_scale = 1;
	size_t A, B, j;
    const dsp_comp *tw_p;

    while(cur_N > 1)
    {
        /* cur_N go down 2 exponents
         * rightshift means faster divison with 2 */
        cur_N >>= 1;

        for(A = 0, B = 0; A < (DSP_FFT_IN_N - 1); A += cur_N)
        {
            B = A + cur_N;
            tw_p = twiddle_lut;

            for(j = 0; j < cur_N; j++)
            {
                if(j)
                {
                    butterfly(&in_R[A], &in_R[B], tw_p);
                    butterfly(&in_L[A], &in_L[B], tw_p);
                }
                else
                {
                    butterfly_w0(&in_R[A], &in_R[B]);
                    butterfly_w0(&in_L[A], &in_L[B]);
                }

                A++;
                B++;
                tw_p += w_scale;
            }
        }

        w_scale <<= 1; // w_scale go up 2 exponents
    }
}

void dsp_reverse_bits(dsp_comp* in_R, dsp_comp* in_L, float* out_R, float* out_L)
{
	const uint16_t *bits = rev_bits_lut;

    for(size_t i = 0, j; i < DSP_FFT_IN_N; i++)
    {
        j = *bits++;

        if(j < DSP_FFT_RES_N)
        {
            normalize_output(&in_R[i], &out_R[j]);
            normalize_output(&in_L[i], &out_L[j]);
        }
    }
}

float *dsp_fft_get_res(bool is_right)
{
    if(is_right) return fft_res_r;
    else return fft_res_l;
}

void dsp_new_data(const uint8_t *data, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        ringbuf[ringbuf_i++] = *data++;

        if(DSP_FFT_BUF_N <= ringbuf_i) ringbuf_i = 0;
    }
}

static void decode_ringbuf_val(size_t *buf_i, dsp_comp *dest, float window_val)
{
    int16_t *i16 = (int16_t*)&ringbuf[*buf_i];
    float data = *i16;
    /* scale down to range [-1..1] */
    data /= (float)INT16_MAX + 1.0f;
    dest->re = data * window_val;
    dest->im = 0;
    *buf_i += AUDIO_SAMPLE_BYTE_LEN;

    if(DSP_FFT_BUF_N <= *buf_i) *buf_i = 0;
}

static void butterfly(dsp_comp* A, dsp_comp* B, const dsp_comp* w)
{
    dsp_comp tmp = {
        .re = A->re - B->re,
        .im = A->im - B->im
    };

    A->re = A->re + B->re;
    A->im = A->im + B->im;

    B->re = (tmp.re * w->re) - (tmp.im * w->im);
    B->im = (tmp.re * w->im) + (tmp.im * w->re);
}

static void butterfly_w0(dsp_comp* A, dsp_comp* B)
{
    dsp_comp tmp = {
        .re = A->re - B->re,
        .im = A->im - B->im
    };

    A->re = A->re + B->re;
    A->im = A->im + B->im;

    /* twiddle 0 is: 1 + 0j
     * means 4pcs mul operation skippable */
    B->re = tmp.re;
    B->im = tmp.im;
}

static void normalize_output(dsp_comp* in, float* out)
{
    /* distance range: [0..DSP_FFT_IN_N] */
	float distance = sqrtf((in->re * in->re) + (in->im * in->im));
    /* convert to range: [0..1] */
    *out = distance / (float)DSP_FFT_IN_N;
}
