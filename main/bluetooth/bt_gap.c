
#include "esp_system.h"
#include "esp_gap_bt_api.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "bt_gap.h"
#include "app.h"


static void gap_led_cfg(uint32_t clk_div, uint32_t duty);
static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);


static const char *TAG = LOG_COLOR("94") "BT_GAP";
static const char *TAGE = LOG_COLOR("94") "BT_GAP" LOG_COLOR_E;


void bt_gap_init()
{
    /* init bluetooth device */
    ERR_CHECK_RESET(esp_bt_gap_set_device_name(BT_DEVICE_NAME));
    ERR_CHECK_RESET(esp_bt_gap_register_callback(gap_callback));
}

void bt_gap_show()
{
    gap_led_cfg(66000, 900);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    ESP_LOGI(TAG, "device discoverable and connectable");
}

void bt_gap_hide()
{
    gap_led_cfg(21000, 2047);
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    ESP_LOGI(TAG, "device not discoverable and not connectable anymore");
}

void bt_gap_led_set_fast()
{
    gap_led_cfg(21000, 2047);
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
                ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
                esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
            } else {
                ESP_LOGE(TAGE, "authentication failed, status: %d", param->auth_cmpl.stat);
            }
            ESP_LOGI(TAG, "link key type of current link is: %d", param->auth_cmpl.lk_type);
            break;
        }
        case ESP_BT_GAP_ENC_CHG_EVT: {
            static const char *str_enc[3] = {"OFF", "E0", "AES"};
            bda = (uint8_t *)param->enc_chg.bda;
            ESP_LOGI(TAG, "Encryption mode to [%02x:%02x:%02x:%02x:%02x:%02x] changed to %s",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], str_enc[param->enc_chg.enc_mode]);
            break;
        }
        /* when GAP mode changed, this event comes */
        case ESP_BT_GAP_MODE_CHG_EVT:
            static const char *mode_string[] = {"active", "hold", "sniff", "park"};
            ESP_LOGI(TAG, "change mode: %s", mode_string[param->mode_chg.mode]);
            break;
        /* when ACL connection completed, this event comes */
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
            bda = (uint8_t *)param->acl_conn_cmpl_stat.bda;
            ESP_LOGI(TAG, "ACL connected complete to [%02x:%02x:%02x:%02x:%02x:%02x], status: 0x%x",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_conn_cmpl_stat.stat);
            break;
        /* when ACL disconnection completed, this event comes */
        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
            bda = (uint8_t *)param->acl_disconn_cmpl_stat.bda;
            ESP_LOGI(TAG, "ACL disconnected complete from [%02x:%02x:%02x:%02x:%02x:%02x], reason: 0x%x",
                    bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_disconn_cmpl_stat.reason);
            break;
        case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
            char tmp[50] = {0};
            char *cur = tmp;
            bool first = true;
            for(int i = 0; i < param->config_eir_data.eir_type_num; i++)
            {
                if(!first) cur += sprintf(cur, ", ");
                cur += sprintf(cur, "%d", param->config_eir_data.eir_type[i]);
                if(first) first = false;
            }
            ESP_LOGI(TAG, "EIR data: [%d] = %s", param->config_eir_data.eir_type_num, tmp);
            break;
        /* others */
        default:
            ESP_LOGI(TAG, "event: %d", event);
            break;
    }
}
