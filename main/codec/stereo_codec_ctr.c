
#include "driver/i2c.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"

#include "main.h"
#include "stereo_codec.h"
#include "stereo_codec_reg.h"


#define BIT_ON(bitN)        1<<bitN
#define BIT_SH(bitN, val)   val<<bitN


static void setup_I2C();
static esp_err_t set_reg(uint8_t reg, uint16_t data);
static esp_err_t config_WM8960();

void stereo_codec_control_init()
{
    /* Initialize I2C peripheral */
    setup_I2C();
    /* Initialize audio codec */
    config_WM8960();
}

void stereo_codec_set_volume(uint8_t vol)
{
    set_reg(0x02, vol & 0x7F);
    set_reg(0x03, (vol & 0x7F) | 0x100);
}

static void setup_I2C()
{
    const i2c_config_t i2c_cfg = {
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .mode = I2C_MODE_MASTER,
        /* on waveshare WM8960 audio board there are 10k pullup resistors */
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 420420,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_PERIPH_NUM, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PERIPH_NUM, I2C_MODE_MASTER, 0, 0, 0));
}

static esp_err_t set_reg(uint8_t reg, uint16_t data)
{
    uint8_t write_buf[2] = {0};
    write_buf[0] = (reg << 1) | ((data >> 8) & 1);
    write_buf[1] = (uint8_t) (data & 0xFF);

    esp_err_t res;
    uint8_t tryN = 1;

    while(1)
    {
        res = i2c_master_write_to_device(I2C_PERIPH_NUM, STEREO_CODEC_I2C_ADDRESS, write_buf, 2, pdMS_TO_TICKS(20));

        if(res == ESP_OK)
        {
            ESP_LOGI(LOG_STEREO_CODEC, "reg %02X set OK after %d try", reg, tryN);
            break;
        }
        else
        {
            if(++tryN > 15)
            {
                ESP_LOGE(LOG_STEREO_CODEC, "reg %02X set failed:", reg);
                break;
            }
        }
    }

    return res;
}

static esp_err_t config_WM8960()
{
    ESP_RETURN_ON_ERROR(set_reg(R15_Reset, 0), LOG_STEREO_CODEC, "reset device failed");

    set_reg(R25_PowerManagment_1,
            BIT_SH(7, 0b11) // fast start-up
            | BIT_ON(6));   // VREF on

    set_reg(R7_AudioInterface_1, 2); // 16 bits audio data word len, I2S Format

    set_reg(R52_PLL_1,
            BIT_ON(5)   // Fractional mode
            | BIT_ON(4) // MCLK / 2 --> PLL (24MHz / 2 = 12MHz)
            | 7);       // PLLN = 7

    set_reg(R53_PLL_2, 0x86); // the values are from a clocking table in WM8960 datasheet
    set_reg(R54_PLL_3, 0xC2);
    set_reg(R55_PLL_4, 0x26);

    set_reg(R4_Clocking_1,
            BIT_SH(1, 0b10) // SYSCLK / 2
            | 1);           // PLL clock selected

    set_reg(R34_LeftOutMix,
            BIT_ON(8)               // route left DAC to output
            | BIT_SH(4, 0b111));    // reduce LINPUT3 vol
    set_reg(R37_RightOutMix,
            BIT_ON(8)               // route right DAC to output
            | BIT_SH(4, 0b111));    // reduce RINPUT3 vol

    set_reg(R45_LeftBypass, BIT_SH(4, 0b111)); // reduce Left Input Boost Mixer vol
    set_reg(R46_RightBypass, BIT_SH(4, 0b111)); // reduce Right Input Boost Mixer vol

    set_reg(R2_LOUT1Volume, 0x78); // set HP_L -1dB vol
    set_reg(R3_ROUT1Volume, 0x78 | BIT_ON(8)); // set HP_R -1dB vol + vol update
    // set_reg(R2_LOUT1Volume, 0);
    // set_reg(R3_ROUT1Volume, BIT_ON(8));

    set_reg(R10_LeftDACVolume, 0xFF); // DAC max vol
    set_reg(R11_RightDACVolume, 0xFF | BIT_ON(8)); // DAC max vol, vol update
    // set_reg(R10_LeftDACVolume, 0);
    // set_reg(R11_RightDACVolume, BIT_ON(8));

    set_reg(R5_ADCAndDACControl_1, 0); // unmute DAC

    // set_reg(R28_AntiPop_1, BIT_ON(4) | BIT_ON(3) | BIT_ON(2));
    // set_reg(R29_AntiPop_2, BIT_ON(6));

    set_reg(R26_PowerManagment_2,
            BIT_ON(8)   // DAC L
            | BIT_ON(7) // DAC R
            | BIT_ON(6) // HP L
            | BIT_ON(5) // HP R
            | 1);       // PLL on
    set_reg(R47_PowerManagement_3,
            BIT_ON(3)       // LOMIX on
            | BIT_ON(2));   // ROMIX on

    // vTaskDelay(pdMS_TO_TICKS(666));
    // set_reg(R28_AntiPop_1, 0);
    // set_reg(R29_AntiPop_2, 0);
    return ESP_OK;
}
