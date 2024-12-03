
#include "esp_gap_bt_api.h"

#include "app_config.h"
#include "app_tools.h"
#include "bt_profiles.h"
#include "led_std.h"


static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);


static const char *TAG = LOG_COLOR("94") "BT_GAP";
static const char *TAGE = LOG_COLOR("94") "BT_GAP" LOG_COLOR_E;


void bt_gap_init()
{
    /* init bluetooth device */
    ERR_CHECK_RESET(esp_bt_gap_register_callback(gap_callback));
    ERR_CHECK_RESET(esp_bt_gap_set_device_name(BT_DEVICE_NAME));
    ESP_LOGI(TAG, "init OK");
}

void bt_gap_show()
{
    sled_set(SLED_MODE_SLOW);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    ESP_LOGI(TAG, "device discoverable and connectable");
}

void bt_gap_hide()
{
    sled_set(SLED_MODE_DIM);
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    ESP_LOGI(TAG, "device not discoverable and not connectable anymore");
}

static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    /* bluetooth device address */
    uint8_t *bda = NULL;

    switch(event)
    {
        /* when authentication completed, this event comes */
        case ESP_BT_GAP_AUTH_CMPL_EVT: {
            bda = (uint8_t *)param->auth_cmpl.bda;

            if(param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "authentication success: %s [%02X:%02X:%02X:%02X:%02X:%02X]", param->auth_cmpl.device_name, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            }
            else
            {
                ESP_LOGE(TAGE, "authentication failed [%02X:%02X:%02X:%02X:%02X:%02X], status: 0x%X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->auth_cmpl.stat);
            }

            ESP_LOGI(TAG, "link key type of current link is: 0x%X", param->auth_cmpl.lk_type);
            break;
        }
        case ESP_BT_GAP_ENC_CHG_EVT: {
            static const char *str_enc[3] = {"OFF", "E0", "AES"};
            bda = (uint8_t *)param->enc_chg.bda;
            ESP_LOGI(TAG, "encryption mode to [%02X:%02X:%02X:%02X:%02X:%02X] changed to %s", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], str_enc[param->enc_chg.enc_mode]);
            break;
        }
        /* when GAP mode changed, this event comes */
        case ESP_BT_GAP_MODE_CHG_EVT: {
            static const char *mode_string[] = {"active", "hold", "sniff", "park"};
            ESP_LOGI(TAG, "change mode: %s", mode_string[param->mode_chg.mode]);
            break;
        }
        /* when ACL connection completed, this event comes
         * (ACL: Asynchronous Connection-oriented Logical transport) */
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT: {
            sled_set(SLED_MODE_FAST);
            bda = (uint8_t *)param->acl_conn_cmpl_stat.bda;
            ESP_LOGI(TAG, "ACL connected complete to [%02X:%02X:%02X:%02X:%02X:%02X], status: 0x%X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_conn_cmpl_stat.stat);
            break;
        }
        /* when ACL disconnection completed, this event comes */
        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT: {
            bda = (uint8_t *)param->acl_disconn_cmpl_stat.bda;
            ESP_LOGI(TAG, "ACL disconnected complete from [%02X:%02X:%02X:%02X:%02X:%02X], reason: 0x%X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_disconn_cmpl_stat.reason);
            break;
        }
        /* Extended Inquiry Response (EIR) data event
         * this data types will send to remote device when discovering */
        case ESP_BT_GAP_CONFIG_EIR_DATA_EVT: {
            char tmp[50] = {0};
            char *cur = tmp;
            bool first = true;

            for(int i = 0; i < param->config_eir_data.eir_type_num; i++)
            {
                if(!first) cur += sprintf(cur, ", ");
                cur += sprintf(cur, "0x%02X", param->config_eir_data.eir_type[i]);
                if(first) first = false;
            }

            ESP_LOGI(TAG, "configured EIR data types: [%d] = %s", param->config_eir_data.eir_type_num, tmp);
            break;
        }
        default:
            ERR_BAD_CASE(event, "%d");
    }
}
