
#ifndef __MAIN_H__
#define __MAIN_H__


#define BT_DEVICE_NAME  "ESP_SPEAKER"

#define STEREO_CODEC_I2C_ADDRESS 0x1A

/* used I2C peripheral number */
#define I2C_PERIPH_NUM  I2C_NUM_0
/* used I2C pins */
#define I2C_SDA_PIN     GPIO_NUM_21
#define I2C_SCL_PIN     GPIO_NUM_22

/* used I2S peripheral number */
#define I2S_PERIPH_NUM  I2S_NUM_0
/* used I2S pins */
#define I2S_BCLK_PIN    GPIO_NUM_25
#define I2S_WS_PIN      GPIO_NUM_26
#define I2S_DOUT_PIN    GPIO_NUM_27

/* I2S DMA buffer number */
#define I2S_DMA_BUF_N   10
/* I2S frame number in one DMA buffer */
#define I2S_DMA_FRAME_N 240

/* onboard LED */
#define LED_BLUE        GPIO_NUM_2

//GPIO 16 RGB?


#endif /* __MAIN_H__ */
