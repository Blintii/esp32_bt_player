
#include "esp_system.h"
#include "esp_log.h"
#include "esp_a2dp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main.h"
#include "task_hub.h"
#include "bt_a2dp.h"
#include "stereo_codec.h"
#include "bt_gap.h"


static void a2dp_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void a2dp_data_callback(const uint8_t *data, uint32_t len);
static void a2dp_event(uint16_t event, void *p_param);


/* connection state in string */
static const char *s_a2d_conn_state_str[] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
/* audio stream datapath state in string */
static const char *s_a2d_audio_state_str[] = {"Suspended", "Stopped", "Started"};
/* count for audio packet */
static uint32_t s_pkt_cnt = 0;
/* audio stream datapath state */
static esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;


void bt_a2dp_init()
{
    /* in file: \esp\esp-idf\components\bt\host\bluedroid\btc\profile\std\a2dp\bta_av_co.c
     * in row 84: removed not supported 48kHz sample freq
     * in row 85: removed not supported mono mode
     */
    ESP_ERROR_CHECK(esp_a2d_sink_init());
    esp_a2d_register_callback(&a2dp_callback);
    esp_a2d_sink_register_data_callback(a2dp_data_callback);

    /* Get the default value of the delay value */
    esp_a2d_sink_get_delay_value();
}

static void a2dp_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_PROF_STATE_EVT:
    case ESP_A2D_SNK_PSC_CFG_EVT:
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT: {
        task_hub_bt_app_work_dispatch(a2dp_event, event, param, sizeof(esp_a2d_cb_param_t));
        break;
    }
    default:
        ESP_LOGE(LOG_BT_A2DP, "%s unhandled event: %d", __func__, event);
        break;
    }
}

static void a2dp_data_callback(const uint8_t *data, uint32_t len)
{
    task_hub_ringbuf_send(data, len);

    /* log the number every 100 packets */
    if(++s_pkt_cnt % 100 == 0)
    {
        ESP_LOGI(LOG_BT_A2DP, "audio packet count: %"PRIu32, s_pkt_cnt);
    }
}

static void a2dp_event(uint16_t event, void *p_param)
{
    esp_a2d_cb_param_t *a2d = NULL;

    switch(event)
    {
        /* when connection state changed, this event comes */
        case ESP_A2D_CONNECTION_STATE_EVT:
        {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            uint8_t *bda = a2d->conn_stat.remote_bda;
            ESP_LOGI(LOG_BT_A2DP, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                s_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            
            if(a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
            {
                bt_gap_show();
                task_hub_I2S_del();
            } 
            else if(a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
            {
                bt_gap_hide();
                task_hub_I2S_create();
            }
            else if(a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
            {
            }

            break;
        }
        /* when audio stream transmission state changed, this event comes */
        case ESP_A2D_AUDIO_STATE_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            ESP_LOGI(LOG_BT_A2DP, "transmission stream state: %s", s_a2d_audio_state_str[a2d->audio_stat.state]);
            s_audio_state = a2d->audio_stat.state;
            if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
                s_pkt_cnt = 0;
            }
            break;
        }
        /* when audio codec is configured, this event comes */
        case ESP_A2D_AUDIO_CFG_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            /* for now only SBC stream is supported */
            if(a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC)
            {
                ESP_LOGI(LOG_BT_A2DP, "incoming audio stream codec: SBC");
                int sample_rate = 16000;
                char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];

                if (oct0 & (0x01 << 6)) sample_rate = 32000;
                else if(oct0 & (0x01 << 5)) sample_rate = 44100;
                else if(oct0 & (0x01 << 4)) sample_rate = 48000;

                if(oct0 & (0x01 << 3)) ESP_LOGE(LOG_BT_A2DP, "NOT SUPPORTED channel mode: mono");

                ESP_LOGI(LOG_BT_A2DP, "configure SBC codec: %x-%x-%x-%x",
                                    a2d->audio_cfg.mcc.cie.sbc[0],
                                    a2d->audio_cfg.mcc.cie.sbc[1],
                                    a2d->audio_cfg.mcc.cie.sbc[2],
                                    a2d->audio_cfg.mcc.cie.sbc[3]);
                
                if(sample_rate != 44100)
                {
                    ESP_LOGE(LOG_BT_A2DP, "NOT SUPPORTED sample rate: %d", sample_rate);
                }
            }
            else
            {
                ESP_LOGE(LOG_BT_A2DP, "NOT SUPPORTED incoming audio stream codec: %d", a2d->audio_cfg.mcc.type);
            }

            break;
        }
        /* when a2dp init or deinit completed, this event comes */
        case ESP_A2D_PROF_STATE_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            if (ESP_A2D_INIT_SUCCESS == a2d->a2d_prof_stat.init_state) {
                ESP_LOGI(LOG_BT_A2DP, "PROF STATE: Init Complete");
            } else {
                ESP_LOGI(LOG_BT_A2DP, "PROF STATE: Deinit Complete");
            }
            break;
        }
        /* When protocol service capabilities configured, this event comes */
        case ESP_A2D_SNK_PSC_CFG_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            ESP_LOGI(LOG_BT_A2DP, "protocol service capabilities configured: 0x%x ", a2d->a2d_psc_cfg_stat.psc_mask);
            if (a2d->a2d_psc_cfg_stat.psc_mask & ESP_A2D_PSC_DELAY_RPT) {
                ESP_LOGI(LOG_BT_A2DP, "Peer device support delay reporting");
            } else {
                ESP_LOGI(LOG_BT_A2DP, "Peer device unsupport delay reporting");
            }
            break;
        }
        /* when set delay value completed, this event comes */
        case ESP_A2D_SNK_SET_DELAY_VALUE_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            if (ESP_A2D_SET_INVALID_PARAMS == a2d->a2d_set_delay_value_stat.set_state) {
                ESP_LOGI(LOG_BT_A2DP, "Set delay report value: fail");
            } else {
                ESP_LOGI(LOG_BT_A2DP, "Set delay report value: success, delay_value: %u * 1/10 ms", a2d->a2d_set_delay_value_stat.delay_value);
            }
            break;
        }
        /* when get delay value completed, this event comes */
        case ESP_A2D_SNK_GET_DELAY_VALUE_EVT: {
            a2d = (esp_a2d_cb_param_t *)(p_param);
            ESP_LOGI(LOG_BT_A2DP, "Get delay report value: delay_value: %u * 1/10 ms", a2d->a2d_get_delay_value_stat.delay_value);
            /* Default delay value plus delay caused by application layer */
            esp_a2d_sink_set_delay_value(a2d->a2d_get_delay_value_stat.delay_value + BT_A2DP_APP_DELAY_VALUE);
            break;
        }
        /* others */
        default:
            ESP_LOGE(LOG_BT_A2DP, "%s unhandled event: %d", __func__, event);
            break;
    }
}