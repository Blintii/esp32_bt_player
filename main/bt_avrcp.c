
#include <string.h>
#include <stdbool.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_avrc_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main.h"
#include "bt_avrcp.h"
#include "task_hub.h"
#include "stereo_codec.h"


static void avrcp_control_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void avrcp_alloc_track_meta_buffer(esp_avrc_ct_cb_param_t *param);
static void avrcp_control_event(uint16_t event, void *p_param);
static void avrcp_notify_event_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter);
static void avrcp_new_track_loaded(void);
static void avrcp_playback_status_changed(void);
static void avrcp_play_pos_changed(void);
static void avrcp_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
static void avrcp_target_event(uint16_t event, void *p_param);
static void volume_set_by_controller(uint8_t volume);


/* playback state in string */
static const char *s_audio_playback_state_str[] = {"Stopped", "Playing", "Paused", "Forward seek", "Reverse seek", "Error"};
/* AVRC target notification capability bit mask */
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
/* local volume value */
static uint8_t s_volume = 0;


void bt_avrcp_init()
{
    /* controller need to init first */
    ESP_ERROR_CHECK(esp_avrc_ct_init());
    ESP_ERROR_CHECK(esp_avrc_ct_register_callback(avrcp_control_callback));

    /* target only can init after controller init complete */
    ESP_ERROR_CHECK(esp_avrc_tg_init());
    ESP_ERROR_CHECK(esp_avrc_tg_register_callback(avrcp_target_callback));

    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
    ESP_ERROR_CHECK(esp_avrc_tg_set_rn_evt_cap(&evt_set));
}

static void avrcp_control_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch(event)
    {
        case ESP_AVRC_CT_METADATA_RSP_EVT:
            avrcp_alloc_track_meta_buffer(param);
            /* fall through */
        case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
            task_hub_bt_app_work_dispatch(avrcp_control_event, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        }
        default:
            ESP_LOGE(LOG_BT_AVRCP, "Invalid AVRC event: %d", event);
            break;
    }
}

static void avrcp_alloc_track_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);

    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;
    rc->meta_rsp.attr_text = attr_text;
}

static void avrcp_control_event(uint16_t event, void *p_param)
{
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event) {
    /* when connection state changed, this event comes */
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(LOG_BT_AVRCP, "AVRC conn_state event: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected) {
            /* get remote supported event_ids of peer AVRCP Target */
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        } else {
            /* clear peer notification capability record */
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }
    /* when passthrough responsed, this event comes */
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        ESP_LOGI(LOG_BT_AVRCP, "AVRC passthrough rsp: key_code 0x%x, key_state %d, rsp_code %d", rc->psth_rsp.key_code,
                    rc->psth_rsp.key_state, rc->psth_rsp.rsp_code);
        break;
    }
    /* when metadata responsed, this event comes */
    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        switch(rc->meta_rsp.attr_id)
        {
            case ESP_AVRC_MD_ATTR_TITLE: ESP_LOGI(LOG_BT_AVRCP, "title: %s", rc->meta_rsp.attr_text); break;
            case ESP_AVRC_MD_ATTR_ARTIST: ESP_LOGI(LOG_BT_AVRCP, "artist: %s", rc->meta_rsp.attr_text); break;
            case ESP_AVRC_MD_ATTR_ALBUM: ESP_LOGI(LOG_BT_AVRCP, "album: %s", rc->meta_rsp.attr_text); break;
            case ESP_AVRC_MD_ATTR_PLAYING_TIME: ESP_LOGI(LOG_BT_AVRCP, "play time: %s", rc->meta_rsp.attr_text); break;
            default:
                ESP_LOGI(LOG_BT_AVRCP, "metadata attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
                break;
        }

        free(rc->meta_rsp.attr_text);
        break;
    }
    /* when notified, this event comes */
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
        ESP_LOGI(LOG_BT_AVRCP, "AVRC event notification: %d", rc->change_ntf.event_id);
        avrcp_notify_event_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
        break;
    }
    /* when feature of remote device indicated, this event comes */
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
        ESP_LOGI(LOG_BT_AVRCP, "AVRC remote features %"PRIx32", TG features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
        break;
    }
    /* when notification capability of peer device got, this event comes */
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
        ESP_LOGI(LOG_BT_AVRCP, "remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
                 rc->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
        avrcp_new_track_loaded();
        avrcp_playback_status_changed();
        avrcp_play_pos_changed();
        break;
    }
    /* others */
    default:
        ESP_LOGE(LOG_BT_AVRCP, "%s unhandled event: %d", __func__, event);
        break;
    }
}

static void avrcp_notify_event_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter)
{
    switch (event_id) {
    /* when new track is loaded, this event comes */
    case ESP_AVRC_RN_TRACK_CHANGE:
        avrcp_new_track_loaded();
        break;
    /* when track status changed, this event comes */
    case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
        uint8_t str_i = event_parameter->playback;

        if(str_i == ESP_AVRC_PLAYBACK_ERROR) str_i = 5;

        ESP_LOGI(LOG_BT_AVRCP, "Playback status changed: %s", s_audio_playback_state_str[str_i]);
        avrcp_playback_status_changed();
        break;
    /* when track playing position changed, this event comes */
    case ESP_AVRC_RN_PLAY_POS_CHANGED:
        ESP_LOGI(LOG_BT_AVRCP, "Play position changed: %"PRIu32"-ms", event_parameter->play_pos);
        avrcp_play_pos_changed();
        break;
    /* others */
    default:
        ESP_LOGI(LOG_BT_AVRCP, "unhandled event: %d", event_id);
        break;
    }
}

static void avrcp_new_track_loaded(void)
{
    /* request metadata */
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE |
                        ESP_AVRC_MD_ATTR_ARTIST |
                        ESP_AVRC_MD_ATTR_ALBUM |
                        ESP_AVRC_MD_ATTR_PLAYING_TIME;
    esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, attr_mask);

    /* register notification if peer support the event_id */
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_TRACK_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE,
                                                   ESP_AVRC_RN_TRACK_CHANGE, 0);
    }
}

static void avrcp_playback_status_changed(void)
{
    /* register notification if peer support the event_id */
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE,
                                                   ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

static void avrcp_play_pos_changed(void)
{
    /* register notification if peer support the event_id */
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_PLAY_POS_CHANGED)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE,
                                                   ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
    }
}

static void avrcp_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
    case ESP_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
        task_hub_bt_app_work_dispatch(avrcp_target_event, event, param, sizeof(esp_avrc_tg_cb_param_t));
        break;
    default:
        ESP_LOGE(LOG_BT_AVRCP, "Invalid AVRC event: %d", event);
        break;
    }
}

static void avrcp_target_event(uint16_t event, void *p_param)
{
    esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

    switch (event)
    {
        /* when connection state changed, this event comes */
        case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
            uint8_t *bda = rc->conn_stat.remote_bda;
            ESP_LOGI(LOG_BT_AVRCP, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                    rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            break;
        }
        /* when passthrough commanded, this event comes */
        case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
            ESP_LOGI(LOG_BT_AVRCP, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
            break;
        }
        /* when absolute volume command from remote device set, this event comes */
        case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
            ESP_LOGI(LOG_BT_AVRCP, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100 / 0x7f);
            volume_set_by_controller(rc->set_abs_vol.volume);
            break;
        }
        /* when notification registered, this event comes */
        case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
            ESP_LOGI(LOG_BT_AVRCP, "AVRC register event notification: %d, param: 0x%"PRIx32, rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
            if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
                esp_avrc_rn_param_t rn_param;
                rn_param.volume = s_volume;
                esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
            }
            break;
        }
        /* when feature of remote device indicated, this event comes */
        case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
            ESP_LOGI(LOG_BT_AVRCP, "AVRC remote features: %"PRIx32", CT features: %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
            break;
        }
        /* others */
        default:
            ESP_LOGE(LOG_BT_AVRCP, "%s unhandled event: %d", __func__, event);
            break;
    }
}

static void volume_set_by_controller(uint8_t volume)
{
    s_volume = volume;
    stereo_codec_set_volume(volume);
}
