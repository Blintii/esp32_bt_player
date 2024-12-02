
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"

#include "app_config.h"
#include "app_tools.h"
#include "tasks.h"
#include "ach.h"
#include "bt_profiles.h"
#include "led_std.h"
#include "led_matrix.h"
#include "wifi.h"
#include "web.h"
#include "dns_server.h"
#include "file_system.h"
#include "storage.h"
#include "dsp.h"
#include "math.h"


static const char *TAG = LOG_COLOR("37") "APP";
static const char *TAGE = LOG_COLOR("37") "APP" LOG_COLOR_E;


void app_main(void)
{
    list_tasks_stack_info();
    ESP_LOGI(TAG,
        "\n       _             _ _"
        "\n      / \\  _   _  __| (_) ___"
        "\n     / _ \\| | | |/ _` | |/ _ \\"
        "\n    / ___ \\ |_| | (_| | | (_) |"
        "\n   /_/   \\_\\__,_|\\__,_|_|\\___/"
        "\n                 ____                 _"
        "\n                |  _ \\ ___  __ _  ___| |_"
        "\n                | |_) / _ \\/ _` |/ __| __|"
        "\n                |  _ <  __/ (_| | (__| |_"
        "\n                |_| \\_\\___|\\__,_|\\___|\\__|"
        "\n");

    esp_log_level_set("gpio", ESP_LOG_WARN);
    esp_log_level_set("BT_LOG", ESP_LOG_WARN);
    esp_log_level_set("DNS", ESP_LOG_WARN);

    ESP_LOGI(TAG, "NVS flash init...");
    /* initialize NVS — it is used to store PHY or RF modul calibration data
     * (used when WiFi or BT enabled) */
    esp_err_t err = nvs_flash_init();

    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAGE, "NVS flash erase...");
        ERR_CHECK_RESET(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ERR_CHECK_RESET(err);
    ESP_LOGI(TAG, "NVS flash init OK");

    /* file system needed for web page files */
    ERR_CHECK(spiffs_init());

    sled_init();
    mled_init();

    /* init application tasks */
    tasks_create();

    ESP_LOGI(TAG, "Bluetooth init...");
    /* classic bluetooth used only
     * so release the memory for Bluetooth Low Energy */
    ERR_CHECK_RESET(esp_bt_mem_release(ESP_BT_MODE_BLE));

    /* BR/EDR configured in ESP menuconfig:
     *  BR: Basic Rate,
     *  EDR: Enhanced Data Rate */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ERR_CHECK_RESET(esp_bt_controller_init(&bt_cfg));
    ERR_CHECK_RESET(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_LOGI(TAG, "bt controller init OK");

    /* in ESP-IDF only 1 host available for Classic BT is bluedroid
     * (ESP32 version of native android BT stack) */
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ERR_CHECK_RESET(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ERR_CHECK_RESET(esp_bluedroid_enable());
    ESP_LOGI(TAG, "bluedroid init OK");

    /* init bluetooth profiles */
    bt_gap_init();
    bt_avrcp_init();
    bt_a2dp_init();
    bt_gap_show();

    ESP_LOGI(TAG, "hotspot init...");
    /* Initialize networking stack */
    ERR_CHECK(esp_netif_init());
    /* Create default event loop needed by the  main app */
    ERR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "netif init OK");
    wifi_ini();
    /* DNS server will redirect all queries to the softAP IP */
    ESP_LOGI(TAG, "DNS server starting...");
    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    ERR_CHECK(start_dns_server(&config));
    ESP_LOGI(TAG, "DNS server started OK");
    /* web server will serve clients with captive portal web page */
    ERR_CHECK(web_start_server());

    storage_config_parse();

    vTaskDelay(pdMS_TO_TICKS(1000));
    list_tasks_stack_info();
    ESP_LOGI(TAG, "exit the main entry point");
}

// if needs callback function when stack overflow
//  redeclare this:
// vApplicationStackOverflowHook()

void list_tasks_stack_info()
{
    ESP_LOGW(TAG, "RTOS STACK INFO");
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;

    /* Take a snapshot of the number of tasks in case it changes while this
        function is executing. */
    uxArraySize = uxTaskGetNumberOfTasks();
    ESP_LOGW(TAG, "running task count: %d", uxArraySize);

    /* Allocate a TaskStatus_t structure for each task. An array could be
        allocated statically at compile time. */
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray == NULL ) ESP_LOGE(TAGE, "NULL port malloc, can't get tasks stack info");
    else
    {
        /* Generate raw status information about each task. */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
        ESP_LOGW(TAG, "provided info task count: %d", uxArraySize);
        ESP_LOGW(TAG, "<taskname>: <smallest free bytes>");

        /* For each populated position in the pxTaskStatusArray array,
        format the raw data as human readable ASCII data. */
        for( x = 0; x < uxArraySize; x++ )
        {
            printf("%s: %ld\n\r", pxTaskStatusArray[x].pcTaskName, pxTaskStatusArray[x].usStackHighWaterMark);
        }

        /* The array is no longer needed, free the memory it consumes. */
        vPortFree( pxTaskStatusArray );
    }

    ESP_LOGW(TAG, "free heap: %ld (min: %ld)", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
}
