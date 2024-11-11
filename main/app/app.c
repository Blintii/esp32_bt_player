
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "driver/ledc.h"

#include "app_tools.h"
#include "app_config.h"
#include "task_hub.h"
#include "stereo_codec.h"
#include "bt_profiles.h"


static const char *TAG = LOG_COLOR("37") "app";
static const char *TAGE = LOG_COLOR("37") "app" LOG_COLOR_E;


void app_main(void)
{
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
    esp_log_level_set("i2c.master", ESP_LOG_NONE);
    esp_log_level_set("BT_LOG", ESP_LOG_WARN);

    /* initialize NVS — it is used to store PHY or RF modul calibration data
       (used when WiFi or BT enabled)*/
    esp_err_t err = nvs_flash_init();

    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAGE, "NVS flash erase...");
        ERR_CHECK_RESET(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ERR_CHECK_RESET(err);

    ledc_timer_config_t ledc_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 100,
        .clk_cfg = LEDC_REF_TICK
    };
    ERR_CHECK_RESET(ledc_timer_config(&ledc_cfg));

    ledc_channel_config_t ledc_ch_cfg = {
        .gpio_num = PIN_LED_BLUE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };
    ERR_CHECK_RESET(ledc_channel_config(&ledc_ch_cfg));

    /* init application tasks */
    task_hub_tasks_create();

    /* classic bluetooth used only
       so release the memory for Bluetooth Low Energy */
    ERR_CHECK_RESET(esp_bt_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ERR_CHECK_RESET(esp_bt_controller_init(&bt_cfg));
    ERR_CHECK_RESET(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    /* in ESP-IDF only 1 host available for Classic BT is bluedroid
       (ESP32 version of native android BT stack) */
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ERR_CHECK_RESET(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ERR_CHECK_RESET(esp_bluedroid_enable());

    /* init bluetooth profiles */
    bt_gap_init();
    bt_avrcp_init();
    bt_a2dp_init();
    bt_gap_show();

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

    if( pxTaskStatusArray == NULL ) ESP_LOGE(TAGE, "NULL port malloc");
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
}
