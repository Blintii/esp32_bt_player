
#include "driver/i2c.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "app_tools.h"
#include "ach.h"
#include "ach_reg.h"
#include "bt_profiles.h"
#include "led_std.h"


static void ach_ctr_I2C_init();
static esp_err_t WM8960_set(uint8_t reg, ach_reg_WM8960 data);
static void WM8960_vol_hp(uint8_t vol);
static void WM8960_vol_dac(uint8_t vol);
static void WM8960_prepare_reset();
static void WM8960_config();


static const char *TAG = LOG_COLOR("95") "CODEC" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("95") "CODEC" LOG_COLOR_E;
extern const uint8_t lut_out1vol[128];
extern const uint8_t lut_dacvol[128];
static uint8_t vol_cur = 0;
static uint8_t vol_hp_last = 0;
static uint8_t vol_dac_last = 0;
static bool muted = true;


void ach_control_init()
{
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, 0);
    /* Initialize I2C peripheral */
    ach_ctr_I2C_init();
    /* Initialize audio codec */
    WM8960_config();
}

void ach_set_volume(uint8_t vol)
{
    uint8_t vol_i = 0;

    if(vol < 127) vol_i = vol;
    else vol_i = 127;

    vol_cur = vol_i;
    WM8960_vol_dac(lut_dacvol[vol_cur]);

    /* if muted, the volume change cause unwanted unmuting
     * in the unmuting function hp volume will synced */
    if(!muted) WM8960_vol_hp(lut_out1vol[vol_cur]);
}

void ach_unmute()
{
    ach_reg_WM8960 reg;
    /* DAC has own control bit for mute */
    reg.raw = 0;
    // reg.R5_ADCAndDACControl_1.DACMU = 0: unmute DAC
    WM8960_set(5, reg);

    /* HP zero volume set/unset can perform analog mute */
    WM8960_vol_hp(lut_out1vol[vol_cur]);
    muted = false;
    ESP_LOGW(TAG, "unmute OK");
}

void ach_mute()
{
    ach_reg_WM8960 reg;
    /* DAC has own control bit for mute */
    reg.raw = 0;
    reg.R5_ADCAndDACControl_1.DACMU = 1; // mute DAC
    WM8960_set(5, reg);

    /* HP zero volume set/unset can perform analog mute */
    WM8960_vol_hp(0);
    muted = true;
    ESP_LOGW(TAG, "mute OK");
}

static void ach_ctr_I2C_init()
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

static esp_err_t WM8960_set(uint8_t reg, ach_reg_WM8960 data)
{
    /* I2C set register data frame structure
     * ---+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----
     * 16 | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1
     * ---------------------------------+--------------------------------------------
     *      register number [7bit]      |            data value [9bit]
     * ---------------------------------+----+---------------------------------------
     *         write buf byte 0 [8bit]       |        write buf byte 1 [8bit]
     */
    uint8_t write_buf[2] = {0};
    write_buf[0] = (reg << 1) | ((data.raw >> 8) & 1);
    write_buf[1] = (uint8_t) (data.raw & 0xFF);

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
            led_std_set(LED_STD_MODE_FAST);

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

static void WM8960_vol_hp(uint8_t vol)
{
    /*
     * LINE PROTECTION
     *
     * 0dB volume is: 120
     * !!! but WM8960 can set volume larger then 0dB
     * !!!  up to +6dB volume which is: 127
     * !!! can cause DAMAGE on headphone/amplifier input on the line */
    if(vol > 120) vol = 120;

    if(vol_hp_last == vol) return;

    ESP_LOGW("HP", "%d", vol);
    ach_reg_WM8960 reg;
    /* headphone out volume */
    reg.raw = 0;
    reg.R2_LOUT1Volume.LOUT1VOL = vol; // set HP_L vol
    reg.R2_LOUT1Volume.LO1ZC = 1; // zero cross volume change
    WM8960_set(2, reg);
    reg.raw = 0;
    reg.R3_ROUT1Volume.ROUT1VOL = vol; // set HP_R vol
    reg.R3_ROUT1Volume.RO1ZC = 1; // zero cross volume change
    reg.R3_ROUT1Volume.OUT1VU = 1; // vol update
    WM8960_set(3, reg);

    vol_hp_last = vol;
}

static void WM8960_vol_dac(uint8_t vol)
{
    if(vol_dac_last == vol) return;

    ESP_LOGW("DAC", "%d", vol);
    ach_reg_WM8960 reg;
    /* DAC volume */
    reg.raw = 0;
    reg.R10_LeftDACVolume.LDACVOL = vol; // set DAC_L vol
    WM8960_set(10, reg);
    reg.raw = 0;
    reg.R11_RightDACVolume.RDACVOL = vol; // set DAC_R vol
    reg.R11_RightDACVolume.DACVU = 1; // vol update
    WM8960_set(11, reg);

    vol_dac_last = vol;
}

static void WM8960_prepare_reset()
{
    ach_reg_WM8960 reg;
    /* used for reduce pops when change output enable/disable mode */
    reg.raw = 0;
    reg.R28_AntiPop_1.SOFT_ST = 1;
    reg.R28_AntiPop_1.BUFIOEN = 1;
    reg.R28_AntiPop_1.BUFDCOPEN = 1;
    reg.R28_AntiPop_1.POBCTRL = 1;
    WM8960_set(28, reg);

    reg.raw = 0;
    // reg.R25_PowerManagment_1.VMIDSEL = 0: turn OFF mode
    WM8960_set(25, reg);

    reg.raw = 0;
    reg.R29_AntiPop_2.DISOP = 1; // enable discharge headphone capacitors
    reg.R29_AntiPop_2.DRES = 0b11; // 150Ω used
    WM8960_set(29, reg);

    /* waiting for capacitors discharging, to avoid pop */
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static void WM8960_config()
{
    WM8960_prepare_reset();

    ach_reg_WM8960 reg;
    reg.raw = 0;
    reg.R15_Reset = 1;

    while(ESP_OK != WM8960_set(15, reg))
    {
        ESP_LOGE(TAGE, "reset WM8960 failed");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGW(TAG, "try again reset WM8960...");
    }

    ESP_LOGI(TAG, "WM8960 reset OK");

    /* used for reduce pops when change output enable/disable mode */
    reg.raw = 0;
    reg.R28_AntiPop_1.SOFT_ST = 1;
    reg.R28_AntiPop_1.BUFIOEN = 1;
    reg.R28_AntiPop_1.BUFDCOPEN = 1;
    reg.R28_AntiPop_1.POBCTRL = 1;
    WM8960_set(28, reg);

    reg.raw = 0;
    // reg.R7_AudioInterface_1.WL = 0: 16 bits audio data word len
    reg.R7_AudioInterface_1.FORMAT = 0b10; // I2S Format
    WM8960_set(7, reg);

    reg.raw = 0;
    reg.R52_PLL_1.SDM = 1; // Fractional mode
    reg.R52_PLL_1.PLLPRESCALE = 1; // MCLK / 2 --> PLL (24MHz / 2 = 12MHz)
    reg.R52_PLL_1.PLLN = 7;
    WM8960_set(52, reg);

    /* the values are from a clocking table in WM8960 datasheet */
    reg.raw = 0;
    reg.R53_PLL_2.PLLK = 0x86;
    WM8960_set(53, reg);
    reg.raw = 0;
    reg.R54_PLL_3.PLLK = 0xC2;
    WM8960_set(54, reg);
    reg.raw = 0;
    reg.R55_PLL_4.PLLK = 0x26;
    WM8960_set(55, reg);

    reg.raw = 0;
    reg.R4_Clocking_1.CLKSEL = 1; // PLL clock selected
    reg.R4_Clocking_1.SYSCLKDIV = 0b10; // SYSCLK / 2
    // reg.R4_Clocking_1.DACDIV = 0: SYSCLK / 256
    WM8960_set(4, reg);

    /* set DAC volume to 0, because reset set the max volume */
    reg.raw = 0;
    // reg.R10_LeftDACVolume.LDACVOL = 0: set DAC_L vol
    WM8960_set(10, reg);
    reg.raw = 0;
    // reg.R11_RightDACVolume.RDACVOL = 0: set DAC_R vol
    reg.R11_RightDACVolume.DACVU = 1; // vol update
    WM8960_set(11, reg);

    reg.raw = 0;
    reg.R6_ADCAndDACControl_2.DACMR = 1; // enable soft mute slow ramp
    reg.R6_ADCAndDACControl_2.DACSMM = 1; // enable volume ramp up when unmute
    WM8960_set(6, reg);

    reg.raw = 0;
    reg.R23_AdditionalControl_1.TOEN = 1; // enable vol update timeout clock
    reg.R23_AdditionalControl_1.VSEL = 0b11; // 3.3V bias (stay default)
    reg.R23_AdditionalControl_1.TSDEN = 1; // thermal shutdown enable (stay default)
    WM8960_set(23, reg);

    /* used to turn on right/left output mixer */
    reg.raw = 0;
    reg.R47_PowerManagement_3.ROMIX = 1;
    reg.R47_PowerManagement_3.LOMIX = 1;
    WM8960_set(47, reg);

    reg.raw = 0;
    reg.R34_LeftOutMix.LI2LOVOL = 0b111; // reduce LINPUT3 vol
    reg.R34_LeftOutMix.LD2LO = 1; // route left DAC to output
    WM8960_set(34, reg);
    reg.raw = 0;
    reg.R37_RightOutMix.RI2ROVOL = 0b111; // reduce RINPUT3 vol
    reg.R37_RightOutMix.RD2RO = 1; // route right DAC to output
    WM8960_set(37, reg);

    reg.raw = 0;
    reg.R26_PowerManagment_2.PLLEN = 1; // PLL power on
    reg.R26_PowerManagment_2.ROUT1 = 1; // HP_R power on
    reg.R26_PowerManagment_2.LOUT1 = 1; // HP_L power on
    reg.R26_PowerManagment_2.DACR = 1; // DAC_R power on
    reg.R26_PowerManagment_2.DACL = 1; // DAC_L power on
    WM8960_set(26, reg);

    reg.raw = 0;
    reg.R25_PowerManagment_1.VREF = 1; // VREF power on
    reg.R25_PowerManagment_1.VMIDSEL = 0b01; // playback mode
    WM8960_set(25, reg);

    ESP_LOGI(TAG, "WM8960 config OK");
}
