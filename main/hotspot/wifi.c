/* Captive Portal Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "app_tools.h"
#include "wifi.h"


static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_netif_create_wifi_ap();
static void wifi_init_softap();


static const char *TAG = "wifi_ap";
static const char *TAGE = "wifi_ap";


void wifi_ini()
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("wifi_init", ESP_LOG_WARN);
    ESP_LOGI(TAG, "init...");

    // Initialize Wi-Fi including netif
    wifi_netif_create_wifi_ap();

    // Initialise ESP32 in SoftAP mode
    wifi_init_softap();

    esp_wifi_set_max_tx_power(8); // 2dBM..20dBm -> (8..84)

    // TODO: if ESP-IDF v5.4 released: Configure DNS-based captive portal
    // dhcp_set_captiveportal_url();
    ESP_LOGI(TAG, "init OK");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if(event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if(event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static void wifi_netif_create_wifi_ap()
{
    const esp_netif_ip_info_t ip_config = {
        .ip = { .addr = WIFI_IP },
        .gw = { .addr = WIFI_IP },
        .netmask = { .addr = ESP_IP4TOADDR( 255, 255, 255, 0) },
    };
    const esp_netif_inherent_config_t ap_config =
    {
        .flags = (esp_netif_flags_t)(ESP_NETIF_IPV4_ONLY_FLAGS(ESP_NETIF_DHCP_SERVER) | ESP_NETIF_FLAG_AUTOUP),
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
        .ip_info = &ip_config,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = "WIFI_AP_DEF",
        .if_desc = "ap",
        .route_prio = 10,
        .bridge_info = NULL
    };
    const esp_netif_config_t cfg =
    {
        .base = &ap_config,
        .driver = NULL,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_AP
    };

    esp_netif_t *netif = esp_netif_new(&cfg);
    ERR_IF_NULL_RETURN(netif);
    ERR_CHECK_RETURN(esp_netif_attach_wifi_ap(netif));
    ERR_CHECK_RETURN(esp_wifi_set_default_wifi_ap_handlers());
}

static void wifi_init_softap(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ERR_CHECK_RETURN(esp_wifi_init(&cfg));
    ERR_CHECK_RETURN(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASSWORD,
            .max_connection = WIFI_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = 7
        },
    };

    if(strlen(WIFI_PASSWORD) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ERR_CHECK_RETURN(esp_wifi_set_mode(WIFI_MODE_AP));
    ERR_CHECK_RETURN(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ERR_CHECK_RETURN(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. %sSSID: '%s' password: '%s'", LOG_BOLD("95"), WIFI_SSID, WIFI_PASSWORD);
}
