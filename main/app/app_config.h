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
/* used LED strip pins */
#define PIN_LED_STRIP_0     GPIO_NUM_12 // VDD3P3_RTC
#define PIN_LED_STRIP_1     GPIO_NUM_14 // VDD3P3_RTC

/* used I2C peripheral number */
#define I2C_PERIPH_NUM      I2C_NUM_0
/* audio codec address in I2C bus */
#define I2C_ADDRESS_CODEC   0x1A // from WM8960 datasheet

/* used I2S peripheral number */
#define I2S_PERIPH_NUM      I2S_NUM_0
/* I2S DMA buffer number */
#define I2S_DMA_BUF_N       10
/* I2S frame number in one DMA buffer */
#define I2S_DMA_FRAME_N     240


#endif /* __APP_CONFIG_H__ */
