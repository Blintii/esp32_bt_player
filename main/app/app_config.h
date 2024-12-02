/*
 * Application main configuration
 */

#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__


#define BT_DEVICE_NAME      "ESP_SPEAKER"

/* used I2C pins */
#define PIN_I2C_SDA         GPIO_NUM_23 // VDD3P3_CPU
#define PIN_I2C_SCL         GPIO_NUM_22 // VDD3P3_CPU

/* used I2S pins */
#define PIN_I2S_BCLK        GPIO_NUM_25 // VDD3P3_RTC
#define PIN_I2S_WS          GPIO_NUM_33 // VDD3P3_RTC
#define PIN_I2S_DOUT        GPIO_NUM_32 // VDD3P3_RTC

/* onboard blue LED */
#define PIN_LED_BLUE        GPIO_NUM_2 // VDD3P3_RTC
/* used matrix LED strip pins */
#define PIN_MLED_STRIP_0    GPIO_NUM_12 // VDD3P3_RTC
#define PIN_MLED_STRIP_1    GPIO_NUM_14 // VDD3P3_RTC
#define MLED_STRIP_N        2

/* used I2C peripheral number */
#define I2C_PERIPH_NUM      I2C_NUM_0
/* audio codec address in I2C bus */
#define I2C_ADDRESS_CODEC   0x1A // from WM8960 datasheet

/* used audio config */
#define AUDIO_SAMPLE_BYTE_LEN sizeof(uint16_t)
#define AUDIO_SAMPLE_BIT_LEN (AUDIO_SAMPLE_BYTE_LEN * 8)
#define AUDIO_CHANNEL_N     2 // 2 -> left, right (stereo)
#define I2S_SLOT_MODE       I2S_SLOT_MODE_STEREO

/* used I2S peripheral number */
#define I2S_PERIPH_NUM      I2S_NUM_0
/* I2S DMA buffer number
 *  1: laggy, dropping sound
 *  increasing value getting faster I2S channel writing */
#define I2S_DMA_BUF_N       6
/* I2S frame size in one DMA buffer
 *  it should be the multiple of audio sample byte length */
#define I2S_DMA_BUF_SIZE    (400 * AUDIO_SAMPLE_BYTE_LEN)

/* the length of buffer between
 * Bluetooth stack and I2S DMA channel memory
 *  it should be the multiple of audio packet size
 *  audio packet size: sample size * channel number */
#define AUDIO_BUF_LEN       (3200 * (AUDIO_SAMPLE_BYTE_LEN * AUDIO_CHANNEL_N))
/*
 * the trigger level when started writing the buffered
 * audio packets to I2S DMA memory buffer
 * usually audio samples size in 1 Bluetooth packet
 *  - in windows: 640 (2560 bytes)
 *  - in android: 1024 (4096 bytes)
 *
 *               |   1. |   2. |   3. |   4. |   5. |   6.
 * --------------+------+------+------+------+------+------
 *  case windows |  640 | 1280 | 1920 | 2560 | 3200 | 3840
 *  case android | 1024 | 2048 | 3072 | 4096 | 5120 | 6144
 */
#define AUDIO_BUF_TRIGGER_LEVEL (1000 * (AUDIO_SAMPLE_BYTE_LEN * AUDIO_CHANNEL_N))
/* this number control how many data is send once for the I2S from the buffered audio data
 * too large value increase packet dropping but too small value not enough
 * while next audio data sent to I2S */
#define AUDIO_BUF_RECEIVE_SIZE (480 * (AUDIO_SAMPLE_BYTE_LEN * AUDIO_CHANNEL_N))
/* the number how many time sending data to I2S while blocking */
#define AUDIO_BUF_MAX_FLUSH_N 30


#endif /* __APP_CONFIG_H__ */
