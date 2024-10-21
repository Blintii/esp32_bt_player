
#include "driver/i2c_master.h"
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


static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;


void stereo_codec_control_init()
{
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
    ESP_LOGW("DAC", "%d: %d", vol_i, lut_dacvol[vol_i]);
    set_reg(R2_LOUT1Volume, lut_out1vol[vol_i]); // set HP_L vol
    set_reg(R3_ROUT1Volume, lut_out1vol[vol_i] | BIT_ON(8)); // set HP_R vol + vol update

    // set_reg(R10_LeftDACVolume, lut_dacvol[vol_i]); // DAC vol
    // set_reg(R11_RightDACVolume, lut_dacvol[vol_i] | BIT_ON(8)); // DAC vol, vol update
}

static void setup_I2C()
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PERIPH_NUM,
        .scl_io_num = PIN_I2C_SCL,
        .sda_io_num = PIN_I2C_SDA,
        /* on waveshare WM8960 audio board there are 10k pullup resistors */
        .flags.enable_internal_pullup = true
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = STEREO_CODEC_I2C_ADDRESS,
        .scl_speed_hz = 370000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
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
        res = i2c_master_transmit(dev_handle, write_buf, 2, 50);

        if(res == ESP_OK)
        {
            if(tryN < 2) ESP_LOGI(LOG_STEREO_CODEC, "reg %02X set OK after %d try", reg, tryN);
            else ESP_LOGW(LOG_STEREO_CODEC, "reg %02X set OK after %d try", reg, tryN);

            break;
        }
        else
        {
            ESP_LOGE(LOG_STEREO_CODEC, "%s", esp_err_to_name(res));

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
    /* used for reduce pops when change output enable/disable mode */
    set_reg(R28_AntiPop_1, BIT_ON(7) | BIT_ON(4) | BIT_ON(3) | BIT_ON(2));
    set_reg(R25_PowerManagment_1, 0); // turn OFF mode
    set_reg(R29_AntiPop_2,
            BIT_ON(6)           // enable discharge headphone capacitors
            | BIT_SH(4, 0b11)); // 150Ω used

    /* waiting for capacitors discharging, to avoid pop */
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_RETURN_ON_ERROR(set_reg(R15_Reset, 0), LOG_STEREO_CODEC, "reset device failed");

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

    set_reg(R2_LOUT1Volume, 0x78); // set HP_L -1dB vol
    set_reg(R3_ROUT1Volume, 0x78 | BIT_ON(8)); // set HP_R -1dB vol + vol update
    // set_reg(R2_LOUT1Volume, 0);
    // set_reg(R3_ROUT1Volume, BIT_ON(8));

    set_reg(R10_LeftDACVolume, 0xFF); // DAC max vol
    set_reg(R11_RightDACVolume, 0xFF | BIT_ON(8)); // DAC max vol, vol update
    // set_reg(R10_LeftDACVolume, 0);
    // set_reg(R11_RightDACVolume, BIT_ON(8));

    set_reg(R5_ADCAndDACControl_1, 0); // unmute DAC

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
    return ESP_OK;
}
