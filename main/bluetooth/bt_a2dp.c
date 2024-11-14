
#include "esp_system.h"
#include "esp_a2dp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "app_tools.h"
#include "tasks.h"
#include "bt_profiles.h"
#include "ach.h"


static void a2dp_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void a2dp_data_callback(const uint8_t *data, uint32_t len);


static const char *TAG = LOG_COLOR("94") "BT_A2DP";
static const char *TAGE = LOG_COLOR("94") "BT_A2DP" LOG_COLOR_E;
/* count for audio packet */
static uint32_t audio_packet_cnt = 0;


void bt_a2dp_init()
{
    /* in file: \esp\esp-idf\components\bt\host\bluedroid\btc\profile\std\a2dp\bta_av_co.c
     * in row 84: removed not supported 48kHz sample freq
     * in row 85: removed not supported mono mode
     * from "bta_av_co_sbc_sink_caps" struct
     */
    ERR_CHECK_RESET(esp_a2d_register_callback(a2dp_callback));
    ERR_CHECK_RESET(esp_a2d_sink_register_data_callback(a2dp_data_callback));
    ERR_CHECK_RESET(esp_a2d_sink_init());
}

static void a2dp_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch(event)
    {
        /* when connection state changed, this event comes */
        case ESP_A2D_CONNECTION_STATE_EVT: {
            /* connection state in string */
            static const char *conn_state_str[] = {"disconnected", "connecting", "connected", "disconnecting"};
            uint8_t *bda = param->conn_stat.remote_bda;
            ESP_LOGI(TAG, "connection state: %s, [%02X:%02X:%02X:%02X:%02X:%02X]",
                conn_state_str[param->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

            if(param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
            {
                bt_gap_show();
                tasks_I2S_del();
                list_tasks_stack_info();
            }
            else if(param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
            {
                bt_gap_hide();
                tasks_I2S_create();
            }
            else if(param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
            {
            }

            break;
        }
        /* when audio stream transmission state changed, this event comes */
        case ESP_A2D_AUDIO_STATE_EVT: {
            /* audio stream datapath state in string */
            static const char *audio_state_str[] = {"suspend", "started"};
            ESP_LOGI(TAG, "transmission stream state: %s", audio_state_str[param->audio_stat.state]);

            // TODO: when started, I2S channel should started

            if(ESP_A2D_AUDIO_STATE_STARTED == param->audio_stat.state)
            {
                audio_packet_cnt = 0;
            }

            break;
        }
        /* when audio codec is configured, this event comes */
        case ESP_A2D_AUDIO_CFG_EVT: {
            /* for now only SBC stream is supported */
            if(param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC)
            {
                ESP_LOGI(TAG, "incoming audio stream codec: SBC");
                int sample_rate = 16000;
                uint8_t *sbc = param->audio_cfg.mcc.cie.sbc;
                char oct0 = sbc[0];

                if(oct0 & (0x01 << 6)) sample_rate = 32000;
                else if(oct0 & (0x01 << 5)) sample_rate = 44100;
                else if(oct0 & (0x01 << 4)) sample_rate = 48000;

                if(oct0 & (0x01 << 3)) ESP_LOGE(TAGE, "NOT SUPPORTED channel mode: mono");

                ESP_LOGI(TAG, "SBC media codec capabilities: 0x %X %X %X %X", sbc[0], sbc[1], sbc[2], sbc[3]);

                if(sample_rate != 44100)
                {
                    ESP_LOGE(TAGE, "NOT SUPPORTED sample rate: %d", sample_rate);
                }
            }
            else
            {
                ESP_LOGE(TAGE, "NOT SUPPORTED incoming audio stream codec: 0x%X", param->audio_cfg.mcc.type);
            }

            break;
        }
        /* when a2dp init or deinit completed, this event comes */
        case ESP_A2D_PROF_STATE_EVT: {
            if(param->a2d_prof_stat.init_state == ESP_A2D_INIT_SUCCESS)
            {
                ESP_LOGI(TAG, "profile state: init success");
            }
            else
            {
                ESP_LOGI(TAG, "profile state: deinit success");
            }

            break;
        }
        /* When protocol service capabilities configured, this event comes */
        case ESP_A2D_SNK_PSC_CFG_EVT: {
            ESP_LOGI(TAG, "protocol service capabilities configured: 0x%x ", param->a2d_psc_cfg_stat.psc_mask);

            if(param->a2d_psc_cfg_stat.psc_mask & ESP_A2D_PSC_DELAY_RPT)
            {
                ESP_LOGI(TAG, "peer device support delay reporting");
            }
            else
            {
                ESP_LOGI(TAG, "peer device unsupport delay reporting");
            }

            break;
        }
        default:
            ESP_LOGE(TAGE, "unhandled event: %d", event);
            ERR_CHECK(true);
            break;
    }
}

static void a2dp_data_callback(const uint8_t *data, uint32_t len)
{
    tasks_ringbuf_send(data, len);

    if(!audio_packet_cnt)
    {
        audio_packet_cnt = 1;
        ESP_LOGW(TAG, "first audio packet received");
    }

    /* log the number every 100 packets */
    // if(++audio_packet_cnt % 100 == 0)
    // {
    //     ESP_LOGI(TAG, "audio packet count: %"PRIu32, audio_packet_cnt);
    // }
}
