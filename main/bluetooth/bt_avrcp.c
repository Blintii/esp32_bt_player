
#include "esp_avrc_api.h"

#include "app_tools.h"
#include "bt_profiles.h"
#include "tasks.h"


static void avrcp_control_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void avrcp_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
static void volume_set_by_controller(uint8_t volume);


static const char *TAG = LOG_COLOR("94") "BT_AVRCP";
static const char *TAGE = LOG_COLOR("94") "BT_AVRCP" LOG_COLOR_E;
/* remote device's supported notification event_ids bit mask */
static esp_avrc_rn_evt_cap_mask_t peer_rn_evt_cap;
/* local volume value */
static uint8_t cur_volume = 0;


void bt_avrcp_init()
{
    ERR_CHECK_RESET(esp_avrc_ct_register_callback(avrcp_control_callback));
    ERR_CHECK_RESET(esp_avrc_tg_register_callback(avrcp_target_callback));

    /* controller (CT) need to init first */
    ERR_CHECK_RESET(esp_avrc_ct_init());
    /* target (TG) only can init after controller init complete */
    ERR_CHECK_RESET(esp_avrc_tg_init());

    /* Register Notification (RN) volume change event capability set
     * connected remote device detect this capability to able to
     * register to localy changed volume notification */
    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
    ERR_CHECK_RESET(esp_avrc_tg_set_rn_evt_cap(&evt_set));
}

static void avrcp_control_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch(event)
    {
        /* when connection state changed, this event comes */
        case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
            uint8_t *bda = param->conn_stat.remote_bda;
            ESP_LOGI(TAG, "CT connection state: %d, [%02X:%02X:%02X:%02X:%02X:%02X]", param->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

            if(param->conn_stat.connected)
            {
                /* get remote supported event_ids of peer AVRCP Target */
                esp_avrc_ct_send_get_rn_capabilities_cmd(BT_AVRCP_TL_GET_CAPS);
            }
            else
            {
                /* clear peer notification capability record */
                peer_rn_evt_cap.bits = 0;
            }
            break;
        }
        /* when passthrough responsed, this event comes */
        case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
            ESP_LOGW(TAGE, "passthrough responsed: key_code 0x%X, key_state %d, rsp_code %d", param->psth_rsp.key_code, param->psth_rsp.key_state, param->psth_rsp.rsp_code);
            break;
        }
        /* when feature of remote device indicated, this event comes */
        case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
            ESP_LOGI(TAG, "CT features as TG remote: 0x%02lX - 0x%02X", param->rmt_feats.feat_mask, param->rmt_feats.tg_feat_flag);
            break;
        }
        /* when notification capability of peer device got, this event comes */
        case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
            ESP_LOGI(TAG, "remote event IDs capability: count %d, bits 0x%X", param->get_rn_caps_rsp.cap_count, param->get_rn_caps_rsp.evt_set.bits);
            peer_rn_evt_cap.bits = param->get_rn_caps_rsp.evt_set.bits;
            break;
        }
        default:
            ERR_BAD_CASE(event, "%d");
            break;
    }
}

static void avrcp_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch(event)
    {
        /* when connection state changed, this event comes */
        case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
            uint8_t *bda = param->conn_stat.remote_bda;
            ESP_LOGI(TAG, "TG connection state: %d, [%02X:%02X:%02X:%02X:%02X:%02X]",
                    param->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            break;
        }
        /* when passthrough commanded, this event comes */
        case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
            ESP_LOGW(TAGE, "passthrough cmd: key_code 0x%X, key_state %d", param->psth_cmd.key_code, param->psth_cmd.key_state);
            break;
        }
        /* when absolute volume command from remote device set, this event comes */
        case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
            ESP_LOGI(TAG, "set absolute volume: %d (%d%%)", param->set_abs_vol.volume, (int)(param->set_abs_vol.volume * 100 / 0x7f));
            volume_set_by_controller(param->set_abs_vol.volume);
            break;
        }
        /* when notification registered, this event comes */
        case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
            static const char *rn_eventid_str[] = {"", "playback status changed", "track changed", "track reached end", "track reached start", "playback pos changed", "batt status changed", "system status changed", "player application setting changed", "now playing content changed", "available players changed", "addressed player changed", "UIDs changed", "volume changed"};
            ESP_LOGI(TAG, "remote device registering notification: %s (0x%02X)", rn_eventid_str[param->reg_ntf.event_id], param->reg_ntf.event_id);

            if(param->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE)
            {
                /* remote device registered to volume change notification
                 * if volume change locally, change response should send to remote device
                 * when change notify sent, new change event can't send while
                 * remote device registering again to this notification
                 */
                ESP_LOGW(TAG, "send volume change register notification response");
                esp_avrc_rn_param_t rn_param;
                rn_param.volume = cur_volume;
                esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
            }

            break;
        }
        /* when feature of remote device indicated, this event comes */
        case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
            ESP_LOGI(TAG, "TG features as CT remote: 0x%02lX - 0x%02X", param->rmt_feats.feat_mask, param->rmt_feats.ct_feat_flag);
            break;
        }
        default:
            ERR_BAD_CASE(event, "%d");
            break;
    }
}

static void volume_set_by_controller(uint8_t volume)
{
    cur_volume = volume;

    tasks_signal signal = {
        .aim_task = TASKS_INST_AUDIO_PLAYER,
        .type = TASKS_SIG_AUDIO_VOLUME,
        .arg = {
            .audio_volume = {
                .volume = volume
            }
        }
    };
    tasks_message(signal);
}
