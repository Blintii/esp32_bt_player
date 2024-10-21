
#include "esp_system.h"
#include "esp_log.h"
#include "esp_gap_bt_api.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main.h"
#include "bt_gap.h"


static void gap_led_cfg(uint32_t clk_div, uint32_t duty);
static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);


void bt_gap_init()
{
    /* init bluetooth device */
    esp_bt_gap_set_device_name(BT_DEVICE_NAME);

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));

    ledc_timer_config_t ledc_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 100,
        .clk_cfg = LEDC_REF_TICK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_cfg));

    ledc_channel_config_t ledc_ch_cfg = {
        .gpio_num = PIN_LED_BLUE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch_cfg));
}

void bt_gap_show()
{
    gap_led_cfg(66000, 900);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    ESP_LOGI(LOG_BT_GAP, "device discoverable and connectable");
}

void bt_gap_hide()
{
    gap_led_cfg(21000, 2047);
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    ESP_LOGI(LOG_BT_GAP, "device not discoverable and not connectable anymore");
}

void bt_gap_led_set_weak()
{
    gap_led_cfg(256, 200);
}

/* 3906 div, 16bit: 1Hz
 * 62500 div, 12bit: 1Hz
 * 625 div, 12bit: 100Hz */
static void gap_led_cfg(uint32_t clk_div, uint32_t duty)
{
    ledc_set_duty_with_hpoint(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_timer_set(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, clk_div, LEDC_TIMER_12_BIT, LEDC_REF_TICK);
    ledc_timer_rst(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
}

static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    /* bluetooth device address */
    uint8_t *bda = NULL;

    switch(event)
    {
        /* when authentication completed, this event comes */
        case ESP_BT_GAP_AUTH_CMPL_EVT: {
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(LOG_BT_GAP, "authentication success: %s", param->auth_cmpl.device_name);
                esp_log_buffer_hex(LOG_BT_GAP, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
            } else {
                ESP_LOGE(LOG_BT_GAP, "authentication failed, status: %d", param->auth_cmpl.stat);
            }
            ESP_LOGI(LOG_BT_GAP, "link key type of current link is: %d", param->auth_cmpl.lk_type);
            break;
        }
        case ESP_BT_GAP_ENC_CHG_EVT: {
            static const char *str_enc[3] = {"OFF", "E0", "AES"};
            bda = (uint8_t *)param->enc_chg.bda;
            ESP_LOGI(LOG_BT_GAP, "Encryption mode to [%02x:%02x:%02x:%02x:%02x:%02x] changed to %s",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], str_enc[param->enc_chg.enc_mode]);
            break;
        }
        /* when GAP mode changed, this event comes */
        case ESP_BT_GAP_MODE_CHG_EVT:
            static const char *mode_string[] = {"active", "hold", "sniff", "park"};
            ESP_LOGI(LOG_BT_GAP, "change mode: %s", mode_string[param->mode_chg.mode]);
            break;
        /* when ACL connection completed, this event comes */
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
            bda = (uint8_t *)param->acl_conn_cmpl_stat.bda;
            ESP_LOGI(LOG_BT_GAP, "ACL connected complete to [%02x:%02x:%02x:%02x:%02x:%02x], status: 0x%x",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_conn_cmpl_stat.stat);
            break;
        /* when ACL disconnection completed, this event comes */
        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
            bda = (uint8_t *)param->acl_disconn_cmpl_stat.bda;
            ESP_LOGI(LOG_BT_GAP, "ACL disconnected complete from [%02x:%02x:%02x:%02x:%02x:%02x], reason: 0x%x",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_disconn_cmpl_stat.reason);
            break;
        case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
            ESP_LOGI(LOG_BT_GAP, "EIR data: %d", param->config_eir_data.eir_type_num);
            break;
        /* others */
        default:
            ESP_LOGI(LOG_BT_GAP, "event: %d", event);
            break;
    }
}
