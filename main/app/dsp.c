
#include "math.h"
#include "string.h"

#include "app_tools.h"
#include "dsp.h"


static dsp_comp twiddle_lut[DSP_FFT_RES_N] = {0};
static float window_lut[DSP_FFT_IN_N] = {0};
static uint16_t rev_bits_lut[DSP_FFT_IN_N] = {0};
static dsp_comp fft_work_r[DSP_FFT_IN_N] = {0};
static dsp_comp fft_work_l[DSP_FFT_IN_N] = {0};
static float fft_res_r[DSP_FFT_RES_N] = {0};
static float fft_res_l[DSP_FFT_RES_N] = {0};

/* ringbuf contains DSP_DATA_LEN width signed values */
static uint8_t ringbuf[DSP_FFT_BUF_N] = {0};
/* ringbuf_i means the oldest data index,
 * which pointed the data that can override first,
 * and than become latest data */
static size_t ringbuf_i = 0;


// #include "string.h"
// static size_t test_in = 0;
// static size_t test_out = 0;
// static char out_str[3000] = {0};
// static char *str_pos = out_str;
// static bool print = false;


static void decode_ringbuf_val(size_t *buf_i, dsp_comp *dest, float window_val);
static void butterfly(dsp_comp* A, dsp_comp* B, dsp_comp* w);
static void butterfly_w0(dsp_comp* A, dsp_comp* B);
static void normalize_output(dsp_comp* in, float* out);

void dsp_init()
{
	float tmp_N = DSP_FFT_IN_N;
	tmp_N--;

	for(size_t i = 0; i < DSP_FFT_IN_N; i++)
	{
        /* Blackman–Harris window implemented
         * from https://en.wikipedia.org/wiki/Window_function#Blackman–Harris_window */
		window_lut[i] = 0.35875f - 0.48829f * cosf((2.0f * M_PI * i) / tmp_N) + 0.14128f * cosf((4.0f * M_PI * i) / tmp_N) - 0.01168f * cosf((6.0f * M_PI * i) / tmp_N);

        for(size_t n = 0; n < DSP_FFT_EXP; n++)
        {
            if(i & (1 << n))
            {
                rev_bits_lut[i] |= 1 << (DSP_FFT_EXP - 1 - n);
            }
        }
	}

	twiddle_lut[0].re = 1;
	twiddle_lut[0].im = 0;
	tmp_N++;

	for(size_t i = 1; i < DSP_FFT_RES_N; i++)
	{
		twiddle_lut[i].re = cosf(i * 2 * M_PI/tmp_N);
		twiddle_lut[i].im = sinf(i * 2 * M_PI/tmp_N);
	}

    dsp_work_buf_init();
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
	float *w_p = window_lut;

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
    dsp_comp *tw_p;

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
	uint16_t *bits = rev_bits_lut;

    for(size_t i = 0, j; i < DSP_FFT_IN_N; i++)
    {
        j = *bits++;

        // if(print) ESP_LOGW(LOG_RESET_COLOR"BIT", "%d", j);

        if(j < DSP_FFT_RES_N)
        {
            normalize_output(&in_R[i], &out_R[j]);
            normalize_output(&in_L[i], &out_L[j]);
        }
    }

    // if(print)
    // {
    //     ESP_LOGW(LOG_COLOR_E"RES", "\n\n%s\n", out_str);
    //     memset(out_str, 0, sizeof(out_str));
    //     str_pos = out_str;
    //     print = false;
    // }
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

    // test_in++;
}

static void decode_ringbuf_val(size_t *buf_i, dsp_comp *dest, float window_val)
{
    int16_t *i16 = (int16_t*)&ringbuf[*buf_i];
    float data = *i16;
    /* scale down to range [-1..1] */
    data /= (float)INT16_MAX + 1.0f;
    dest->re = data * window_val;
    dest->im = 0;
    *buf_i += DSP_DATA_LEN;

    // if(2500 < test_in)
    // {
    //     if(test_out < DSP_FFT_IN_N*2)
    //     {
    //         str_pos += sprintf(str_pos, "%.0f, ", dest->re);
    //         test_out++;
    //     }
    //     else
    //     {
    //         ESP_LOGI("bro", "JEJ\n\n%s\n", out_str);
    //         memset(out_str, 0, sizeof(out_str));
    //         test_out = 0;
    //         test_in = 0;
    //         str_pos = out_str;
    //         print = true;
    //     }
    // }

    if(DSP_FFT_BUF_N <= *buf_i) *buf_i = 0;
}

static void butterfly(dsp_comp* A, dsp_comp* B, dsp_comp* w)
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

    // if(print)
    // {
    //     str_pos += sprintf(str_pos, "%.0f, ", distance);
    // }
}
