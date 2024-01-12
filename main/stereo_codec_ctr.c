
#include "driver/i2c.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"

#include "main.h"
#include "stereo_codec.h"


#define BIT_ON(x) 1<<x


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
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
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
        res = i2c_master_write_to_device(I2C_PERIPH_NUM, STEREO_CODEC_I2C_ADDRESS, write_buf, 2, pdMS_TO_TICKS(100));

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
    /* R15 (0Fh) Reset
     * Writing to this register resets all registers to their default state
     */
    ESP_RETURN_ON_ERROR(set_reg(15, 0), LOG_STEREO_CODEC, "reset device failed");

    /* R4 (04h) Clocking (1)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8:6  | ADCDIV[2:0]   | 000      | 000...110 = SYSCLK / (1.0...6.0 * 256)
     * ------------------------------------------------------------------------------
     *  5:3  | DACDIV[2:0]   | 000      | 000...110 = SYSCLK / (1.0...6.0 * 256)
     * ------------------------------------------------------------------------------
     *  2:1  | SYSCLKDIV[1:0]| 00       | 00 = Divide SYSCLK by 1
     *       |               |          | 10 = Divide SYSCLK by 2
     * ------------------------------------------------------------------------------
     *  0    | CLKSEL        | 0        | 0 = SYSCLK derived from MCLK
     *       |               |          | 1 = SYSCLK derived from PLL output
     */
    set_reg(4, 0b101); // SYSCLK / 2, PLL clock selected

    /* R52 (34h) PLL (1)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8:6  | OPCLKDIV[2:0] | 000      | SYSCLK Output to GPIO Clock Division ratio
     *       |               |          | 000...101 = SYSCLK / 1-2-3-4-5.5-6
     * ------------------------------------------------------------------------------
     *  5    | SDM           | 0        | 0 = Integer mode
     *       |               |          | 1 = Fractional mode
     * ------------------------------------------------------------------------------
     *  4    | PLLPRESCALE   | 0        | 0 = Divide MCLK by 1 before input to PLL
     *       |               |          | 1 = Divide MCLK by 2 before input to PLL
     * ------------------------------------------------------------------------------
     *  3:0  | PLLN[3:0]     | 1000 (8) | Integer (N) part of PLL I/O freq ratio
     */
    set_reg(52, BIT_ON(5)     // Fractional mode
                | BIT_ON(4) // MCLK / 2 --> PLL
                | 7);       // PLLN = 7

    /*  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     * R53 (35h) PLL (2)            31h
     *  7:0  | PLLK[23:16]   | 00110001 | Fractional (K) part of PLL1 I/O freq ratio
     * ------------------------------------------------------------------------------
     * R54 (36h) PLL (3)            26h
     *  7:0  | PLLK[15:8]    | 00100110 | Fractional (K) part of PLL1 I/O freq ratio
     * ------------------------------------------------------------------------------
     * R55 (37h) PLL (4)            E9h
     *  7:0  | PLLK[7:0]     | 11101001 | Fractional (K) part of PLL1 I/O freq ratio
     */
    set_reg(53, 0x86);
    set_reg(54, 0xC2);
    set_reg(55, 0x26);
    
    set_reg(0x07, 2);
    set_reg(0x02, 0x78);
    set_reg(0x03, 0x78 | 0x100);
    set_reg(0x0A, 0x1FF);
    set_reg(0x0B, 0x1FF);
    set_reg(0x22, BIT_ON(8));
    set_reg(0x25, BIT_ON(8));

    /* R25 (19h) Power management (1)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8:7  | VMIDSEL[1:0]  | 00       | Vmid Divider Enable and Select
     *       |               |          | 00 = Vmid disabled (for OFF mode)
     *       |               |          | 01 = 2 x 50kΩ divider enabled (for playback/record)
     *       |               |          | 10 = 2 x 250kΩ divider enabled (for low-power standby)
     *       |               |          | 11 = 2 x 5kΩ divider enabled (for fast start-up)
     * ------------------------------------------------------------------------------
     *  6    | VREF          | 0        | VREF power down/up (necessary for all other functions)
     * ------------------------------------------------------------------------------
     *  5    | AINL          | 0        | Analogue in PGA Left power down/up
     * ------------------------------------------------------------------------------
     *  4    | AINR          | 0        | Analogue in PGA Right power down/up
     * ------------------------------------------------------------------------------
     *  3    | ADCL          | 0        | ADC Left power down/up
     * ------------------------------------------------------------------------------
     *  2    | ADCR          | 0        | ADC Right power down/up
     * ------------------------------------------------------------------------------
     *  1    | MICB          | 0        | MICBIAS power down/up
     * ------------------------------------------------------------------------------
     *  0    | DIGENB        | 0        | Master Clock Disable
     *       |               |          | 0 = Master clock enabled
     *       |               |          | 1 = Master clock disabled
     */
    set_reg(25, BIT_ON(8) | BIT_ON(7) // fast start-up
                | BIT_ON(6));       // VREF on
    
    /* R26 (1Ah) Power management (2)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | DACL          | 0        | DAC left power down/up
     * ------------------------------------------------------------------------------
     *  7    | DACR          | 0        | DAC right power down/up
     * ------------------------------------------------------------------------------
     *  6    | LOUT1         | 0        | LOUT1 headphone left power down/up
     * ------------------------------------------------------------------------------
     *  5    | ROUT1         | 0        | ROUT1 headphone right power down/up
     * ------------------------------------------------------------------------------
     *  4    | SPKL          | 0        | SPK_LP/SPK_LN speaker left power down/up
     * ------------------------------------------------------------------------------
     *  3    | SPKR          | 0        | SPK_RP/SPK_RN speaker right power down/up
     * ------------------------------------------------------------------------------
     *  2 - reserved
     * ------------------------------------------------------------------------------
     *  1    | OUT3          | 0        | OUT3 mono power down/up
     * ------------------------------------------------------------------------------
     *  0    | PLLEN         | 0        | 0 = PLL off
     *       |               |          | 1 = PLL on
     */
    set_reg(26, BIT_ON(8)     // DAC L
                | BIT_ON(7) // DAC R
                | BIT_ON(6) // HP L
                | BIT_ON(5) // HP R
                | 1);       // PLL on

    /* R47 (2Fh) Power management (3)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8:6 - reserved
     * ------------------------------------------------------------------------------
     *  5    | LMIC          | 0        | Left Channel Input PGA disabled/enabled
     * ------------------------------------------------------------------------------
     *  4    | RMIC          | 0        | Right Channel Input PGA disabled/enabled
     * ------------------------------------------------------------------------------
     *  3    | LOMIX         | 0        | Left Output Mixer disabled/enabled Control
     * ------------------------------------------------------------------------------
     *  2    | ROMIX         | 0        | Right Output Mixer disabled/enabled Control
     * ------------------------------------------------------------------------------
     *  1:0 - reserved
     */
    set_reg(47, BIT_ON(3)         // LOMIX on
                | BIT_ON(2));   // ROMIX on

    /* R5 (05h) ADC and DAC Control (1)
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8 - reserved
     * ------------------------------------------------------------------------------
     *  7    | DACDIV2       | 0        | DAC 6dB Attenuate
     *       |               |          | 0 = Disabled (0dB)
     *       |               |          | 1 = -6dB Enabled
     * ------------------------------------------------------------------------------
     *  6:5  | ADCPOL[1:0]   | 00       | ADC polarity control:
     *       |               |          | 00 = Polarity not inverted
     *       |               |          | 01 = ADC L inverted
     *       |               |          | 10 = ADC R inverted
     *       |               |          | 11 = ADC L and R inverted
     * ------------------------------------------------------------------------------
     *  4 - reserved
     * ------------------------------------------------------------------------------
     *  3    | DACMU         | 1        | DAC Digital Soft Mute / No mute (signal active)
     * ------------------------------------------------------------------------------
     *  2:1  | DEEMPH[1:0]   | 00       | De-emphasis Control
     *       |               |          | 00 = No de-emphasis
     *       |               |          | 01 = 32kHz sample rate
     *       |               |          | 10 = 44.1kHz sample rate
     *       |               |          | 11 = 48kHz sample rate
     * ------------------------------------------------------------------------------
     *  0    | ADCHPD        | 0        | ADC High Pass Filter Disable
     *       |               |          | 0 = Enable high pass filter on
     *       |               |          |     left and right channels
     *       |               |          | 1 = Disable high pass filter on
     *       |               |          |     left and right channels
     *       |               |          | 
     */
    set_reg(5, 0); // unmute DAC
    return ESP_OK;
}
