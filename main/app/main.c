
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "main.h"
#include "task_hub.h"
#include "stereo_codec.h"
#include "bt_gap.h"
#include "bt_avrcp.h"
#include "bt_a2dp.h"


#define LOG_MAIN "MAIN"


void app_main(void)
{
    ESP_LOGI(LOG_MAIN, LOG_RESET_COLOR
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
    /* init application tasks */
    task_hub_tasks_create();
    /* wait for I2C setup */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();

    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    /* classic bluetooth used only
       so release the controller memory for Bluetooth Low Energy */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(LOG_MAIN, "%s initialize controller failed: %s", __func__, esp_err_to_name(err));
        return;
    }
    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(LOG_MAIN, "%s enable controller failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();

    if ((err = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
        ESP_LOGE(LOG_MAIN, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(err));
        return;
    }
    if ((err = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(LOG_MAIN, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    /* init bluetooth profiles */
    bt_gap_init();
    bt_avrcp_init();
    bt_a2dp_init();
    bt_gap_show();

    ESP_LOGI(LOG_MAIN, "exit the main entry point");
}
