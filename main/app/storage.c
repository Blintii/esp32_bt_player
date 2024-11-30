
#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"

#include "app_tools.h"
#include "file_system.h"
#include "storage.h"


static const char *TAG = LOG_COLOR("33") "cfg" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("33") "cfg" LOG_COLOR_E;


cJSON *storage_load(void)
{
    ESP_LOGI(TAG, "open " STORAGE_PATH_CONFIG " file...");
    cJSON *res;
    FILE *file = fopen(STORAGE_PATH_CONFIG, "r");

    if(file)
    {
        ESP_LOGI(TAG, "file exist");
        char buf[STORAGE_FILE_SIZE];
        /* size-1 byte long string can be load to buffer,
         * because string ending zero byte needed */
        size_t content_size = fread(buf, 1, STORAGE_FILE_SIZE - 1, file);

        if(*buf)
        {
            buf[content_size] = 0; // string ending zero byte
            ESP_LOGI(TAG, "file content:\n%s", buf);
        }
        else ESP_LOGE(TAGE, "file empty");

        res = cJSON_Parse(buf);
        fclose(file);

        if(res == NULL)
        {
            ESP_LOGE(TAGE, "file content can't parsed to JSON");
            res = cJSON_CreateObject();
        }
    }
    else
    {
        ESP_LOGE(TAGE, STORAGE_PATH_CONFIG " file NOT exist");
        res = cJSON_CreateObject();
    }

    return res;
}

void storage_save(cJSON *json)
{
    ESP_LOGI(TAG, "saving file...");
    FILE *file = fopen(STORAGE_PATH_CONFIG, "w");

    if(file) ESP_LOGI(TAG, "file not NULL");
    else
    {
        ESP_LOGE(TAGE, "file is NULL");
        ERR_IF_NULL_RETURN(file);
    }

    char *res = cJSON_Print(json);

    if(res) ESP_LOGI(TAG, "JSON: %s", res);
    else ESP_LOGE(TAGE, "JSON is NULL");

    fputs(res, file);
    fclose(file);
    free((void *)res);
    ESP_LOGI(TAG, "file closed");
}
