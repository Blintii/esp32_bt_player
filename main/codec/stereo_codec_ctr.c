
#include "driver/i2c.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "app_tools.h"
#include "stereo_codec.h"
#include "stereo_codec_reg.h"
#include "bt_gap.h"


#define BIT_ON(bitN)        1<<bitN
#define BIT_SH(bitN, val)   val<<bitN


static void setup_I2C();
static esp_err_t set_reg(uint8_t reg, uint16_t data);
static esp_err_t config_WM8960();


static const char *TAG = LOG_COLOR("95") "CODEC" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "CODEC" LOG_COLOR_E;


void stereo_codec_control_init()
{
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, 0);
    /* Initialize I2C peripheral */
    setup_I2C();
    /* Initialize audio codec */
    config_WM8960();
}

void stereo_codec_set_volume(uint8_t vol)
{
    uint8_t vol_i = 0;

    if(vol < 127) vol_i = vol;
    else vol_i = 127;

    ESP_LOGW("HP", "%d: %d", vol_i, lut_out1vol[vol_i]);
    // ESP_LOGW("DAC", "%d: %d", vol_i, lut_dacvol[vol_i]);
    // headphone
    set_reg(R2_LOUT1Volume,
            BIT_ON(7)               // zero cross volume change
            | lut_out1vol[vol_i]);  // set HP_L vol
    set_reg(R3_ROUT1Volume,
            BIT_ON(8)               // vol update
            | BIT_ON(7)             // zero cross volume change
            | lut_out1vol[vol_i] ); // set HP_R vol
    // DAC
    // set_reg(R10_LeftDACVolume, lut_dacvol[vol_i]); // set DAC L vol
    // set_reg(R11_RightDACVolume, lut_dacvol[vol_i] | BIT_ON(8)); // set DAC R vol, vol update
}

void stereo_codec_unmute()
{
    set_reg(R5_ADCAndDACControl_1, 0);
    set_reg(R10_LeftDACVolume, 0xFF); // set DAC L vol
    set_reg(R11_RightDACVolume, 0xFF | BIT_ON(8)); // set DAC R vol, vol update
}

void stereo_codec_mute()
{
    set_reg(R5_ADCAndDACControl_1, BIT_ON(3));
    set_reg(R10_LeftDACVolume, 0); // set DAC L vol
    set_reg(R11_RightDACVolume, BIT_ON(8)); // set DAC R vol, vol update
}

static void setup_I2C()
{
    const i2c_config_t i2c_cfg = {
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 577000,
    };
    ERR_CHECK_RESET(i2c_param_config(I2C_PERIPH_NUM, &i2c_cfg));
    ERR_CHECK_RESET(i2c_driver_install(I2C_PERIPH_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGI(TAG, "I2C init OK");
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
        gpio_set_level(GPIO_NUM_5, 0);
        res = i2c_master_write_to_device(I2C_PERIPH_NUM, I2C_ADDRESS_CODEC, write_buf, 2, pdMS_TO_TICKS(10));

        if(res == ESP_OK)
        {
            if(tryN > 1) ESP_LOGW(TAGE, "reg %02X set OK after %d try", reg, tryN);

            break;
        }
        else
        {
            gpio_set_level(GPIO_NUM_5, 1);
            ESP_LOGE(TAGE, "%s", esp_err_to_name(res));
            bt_gap_led_set_fast();

            if(++tryN > 15)
            {
                ESP_LOGE(TAGE, "reg %02X set failed:", reg);
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    return res;
}

static esp_err_t config_WM8960()
{
    /* used for reduce pops when change output enable/disable mode */
    set_reg(R28_AntiPop_1, BIT_ON(7) | BIT_ON(4) | BIT_ON(3) | BIT_ON(2));
    set_reg(R25_PowerManagment_1, 0); // turn OFF mode
    set_reg(R29_AntiPop_2,
            BIT_ON(6)           // enable discharge headphone capacitors
            | BIT_SH(4, 0b11)); // 150Ω used

    /* waiting for capacitors discharging, to avoid pop */
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_RETURN_ON_ERROR(set_reg(R15_Reset, 0), TAGE, "reset device failed");

    /* used for reduce pops when change output enable/disable mode */
    set_reg(R28_AntiPop_1, BIT_ON(7) | BIT_ON(4) | BIT_ON(3) | BIT_ON(2));

    set_reg(R7_AudioInterface_1, 2); // 16 bits audio data word len, I2S Format

    set_reg(R52_PLL_1,
            BIT_ON(5)   // Fractional mode
            | BIT_ON(4) // MCLK / 2 --> PLL (24MHz / 2 = 12MHz)
            | 7);       // PLLN = 7

    /* the values are from a clocking table in WM8960 datasheet */
    set_reg(R53_PLL_2, 0x86);
    set_reg(R54_PLL_3, 0xC2);
    set_reg(R55_PLL_4, 0x26);

    set_reg(R4_Clocking_1,
            BIT_SH(1, 0b10) // SYSCLK / 2
            | 1);           // PLL clock selected

    set_reg(R45_LeftBypass, BIT_SH(4, 0b111)); // reduce Left Input Boost Mixer vol
    set_reg(R46_RightBypass, BIT_SH(4, 0b111)); // reduce Right Input Boost Mixer vol

    set_reg(R10_LeftDACVolume, 0); // DAC mute
    set_reg(R11_RightDACVolume, BIT_ON(8)); // DAC mute, vol update

    set_reg(R6_ADCAndDACControl_2,
            BIT_ON(3)       // enable volume ramp up when unmute
            | BIT_ON(2));   // enable soft mute slow ramp

    set_reg(R23_AdditionalControl_1,
            BIT_ON(8)           // thermal shutdown enable
            | BIT_SH(6, 0b11)   // 3.3V bias
            | 1);               // enable vol update timeout clock

    set_reg(R47_PowerManagement_3,
            BIT_ON(3)       // LOMIX on
            | BIT_ON(2));   // ROMIX on

    set_reg(R34_LeftOutMix,
            BIT_ON(8)               // route left DAC to output
            | BIT_SH(4, 0b111));    // reduce LINPUT3 vol
    set_reg(R37_RightOutMix,
            BIT_ON(8)               // route right DAC to output
            | BIT_SH(4, 0b111));    // reduce RINPUT3 vol

    set_reg(R26_PowerManagment_2,
            BIT_ON(8)   // DAC L
            | BIT_ON(7) // DAC R
            | BIT_ON(6) // HP L
            | BIT_ON(5) // HP R
            | 1);       // PLL on

    set_reg(R25_PowerManagment_1,
            BIT_SH(7, 0b01) // playback
            | BIT_ON(6));   // VREF on

    ESP_LOGI(TAG, "config OK");
    return ESP_OK;
}
