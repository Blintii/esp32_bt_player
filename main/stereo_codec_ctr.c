
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
        /* on waveshare WM8960 audio board there are 10k pullup resistors */
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
    set_reg(52, BIT_ON(5)   // Fractional mode
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
    
    /* R7 (07h) Audio Interface
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | ALRSWAP       | 0        | 1 = channel swap left/right ADC data in audio interface
     *       |               |          | 0 = output left/right ADC data as normal
     * ------------------------------------------------------------------------------
     *  7    | BCLKINV       | 0        | 0 = BCLK not inverted (for master and slave modes)
     *       |               |          | 1 = BCLK inverted (for master and slave modes)
     * ------------------------------------------------------------------------------
     *  6    | MS            | 0        | 0 = Enable slave mode control
     *       |               |          | 1 = Enable master mode control
     * ------------------------------------------------------------------------------
     *  5    | DLRSWAP       | 0        | 0 = output left/right DAC data as normal
     *       |               |          | 1 = channel swap left/right DAC data in audio interface
     * ------------------------------------------------------------------------------
     *  4    | LRP           | 0        | Right, left and I2S modes – LRCLK polarity
     *       |               |          | 0 = normal LRCLK polarity
     *       |               |          | 1 = invert LRCLK polarity
     *       |               |          | DSP Mode – mode A/B select
     *       |               |          | 0 = MSB is available on 2nd BCLK rising edge after LRC rising edge (mode A)
     *       |               |          | 1 = MSB is available on 1st BCLK rising edge after LRC rising edge (mode B)
     * ------------------------------------------------------------------------------
     *  3:2  | WL[1:0]       | 10       | Audio Data Word Length
     *       |               |          | 00 = 16 bits
     *       |               |          | 01 = 20 bits
     *       |               |          | 10 = 24 bits
     *       |               |          | 11 = 32 bits (see Note)
     * ------------------------------------------------------------------------------
     *  1:0  | FORMAT[1:0]   | 10       | 00 = Right justified
     *       |               |          | 01 = Left justified
     *       |               |          | 10 = I2S Format
     *       |               |          | 11 = DSP Mode
     */
    set_reg(7, 2); // 16 bits audio data word len, I2S Format

    /* R2 (02h) LOUT1 volume
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | OUT1VU        | N/A      | Headphone Output PGA Volume Update
     *       |               |          | Writing a 1 to this bit will cause left and right
     *       |               |          | headphone output volumes to be updated (LOUT1VOL and ROUT1VOL)
     * ------------------------------------------------------------------------------
     *  7    | LO1ZC         | 0        | Left Headphone Output Zero Cross Enable
     *       |               |          | 0 = Change gain immediately
     *       |               |          | 1 = Change gain on zero cross only
     * ------------------------------------------------------------------------------
     *  6:0  | LOUT1VOL[6:0] | 0        | LOUT1 Volume
     *       |               |          | 1111111 = +6dB
     *       |               |          | …1dB steps down to
     *       |               |          | 0110000 = -73dB
     *       |               |          | 0101111 to 0000000 = Analogue MUTE
     */
    set_reg(2, 0x78); // set HP_L -1dB vol

    /* R3 (03h) ROUT1 volume
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | OUT1VU        | N/A      | Headphone Output PGA Volume Update
     *       |               |          | Writing a 1 to this bit will cause left and right
     *       |               |          | headphone output volumes to be updated (LOUT1VOL and ROUT1VOL)
     * ------------------------------------------------------------------------------
     *  7    | RO1ZC         | 0        | Right Headphone Output Zero Cross Enable
     *       |               |          | 0 = Change gain immediately
     *       |               |          | 1 = Change gain on zero cross only
     * ------------------------------------------------------------------------------
     *  6:0  | ROUT1VOL[6:0] | 0        | ROUT1 Volume
     *       |               |          | 1111111 = +6dB
     *       |               |          | ... 1dB steps down to
     *       |               |          | 0110000 = -73dB
     *       |               |          | 0101111 to 0000000 = Analogue MUTE
     */
    set_reg(3, 0x78 | BIT_ON(8)); // set HP_R -1dB vol, vol update

    /* R10 (0Ah) Left DAC Volume
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | DACVU         | N/A      | DAC Volume Update
     *       |               |          | Writing a 1 to this bit will cause left and right
     *       |               |          | DAC volumes to be updated (LDACVOL and RDACVOL)
     * ------------------------------------------------------------------------------
     *  7:0  | LDACVOL[7:0]  | 11111111 | Left DAC Digital Volume Control
     *       |               |          | 0000 0000 = Digital Mute
     *       |               |          | 0000 0001 = -127dB
     *       |               |          | 0000 0010 = -126.5dB
     *       |               |          | ... 0.5dB steps up to
     *       |               |          | 1111 1111 = 0dB
     */
    set_reg(10, 0xFF); // max vol

    /* R11 (0Bh) Left DAC Volume
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | DACVU         | N/A      | DAC Volume Update
     *       |               |          | Writing a 1 to this bit will cause left and right
     *       |               |          | DAC volumes to be updated (LDACVOL and RDACVOL)
     * ------------------------------------------------------------------------------
     *  7:0  | RDACVOL[7:0]  | 11111111 | Right DAC Digital Volume Control
     *       |               |          | 0000 0000 = Digital Mute
     *       |               |          | 0000 0001 = -127dB
     *       |               |          | 0000 0010 = -126.5dB
     *       |               |          | ... 0.5dB steps up to
     *       |               |          | 1111 1111 = 0dB
     */
    set_reg(11, 0xFF | BIT_ON(8)); // max vol, vol update

    /* R34 (22h) Left Out Mix
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | LD2LO         | 0        | Left DAC to Left Output Mixer
     *       |               |          | 0 = Disable (Mute)
     *       |               |          | 1 = Enable Path
     * ------------------------------------------------------------------------------
     *  7    | LI2LO         | 0        | LINPUT3 to Left Output Mixer
     *       |               |          | 0 = Disable (Mute)
     *       |               |          | 1 = Enable Path
     * ------------------------------------------------------------------------------
     *  6:4  | LI2LOVOL[2:0] | 101      | LINPUT3 to Left Output Mixer Volume
     *       |               |          | 000 = 0dB
     *       |               |          | ...3dB steps up to
     *       |               |          | 111 = -21dB
     * ------------------------------------------------------------------------------
     *  3:0 - reserved
     */
    set_reg(34, BIT_ON(8)); // route left DAC to output

    /* R37 (25h) Right Out Mix
     * 
     *  bits | label         | default  | description
     * ------------------------------------------------------------------------------
     *  8    | RD2RO         | 0        | Right DAC to Right Output Mixer
     *       |               |          | 0 = Disable (Mute)
     *       |               |          | 1 = Enable Path
     * ------------------------------------------------------------------------------
     *  7    | RI2RO         | 0        | RINPUT3 to Right Output Mixer
     *       |               |          | 0 = Disable (Mute)
     *       |               |          | 1 = Enable Path
     * ------------------------------------------------------------------------------
     *  6:4  | RI2ROVOL[2:0] | 101      | RINPUT3 to Right Output Mixer Volume
     *       |               |          | 000 = 0dB
     *       |               |          | ...3dB steps up to
     *       |               |          | 111 = -21dB
     * ------------------------------------------------------------------------------
     *  3:0 - reserved
     */
    set_reg(37, BIT_ON(8)); // route right DAC to output

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
    set_reg(25, BIT_ON(8) | BIT_ON(7)   // fast start-up
                | BIT_ON(6));           // VREF on
    
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
    set_reg(26, BIT_ON(8)   // DAC L
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
    set_reg(47, BIT_ON(3)       // LOMIX on
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
