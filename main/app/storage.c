
#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"

#include "app_tools.h"
#include "app_config.h"
#include "file_system.h"
#include "storage.h"
#include "lights.h"


static const char *TAG = LOG_COLOR("33") "cfg" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("33") "cfg" LOG_COLOR_E;


static void config_parse_lights(cJSON *cfg);
static void parse_lights_strips(cJSON *cfg_strips);
static void parse_lights_zones(cJSON *cfg_zones, int strip_index);


cJSON *storage_load(void)
{
    ESP_LOGI(TAG, "open " STORAGE_PATH_CONFIG " file...");
    cJSON *res = NULL;
    FILE *file = fopen(STORAGE_PATH_CONFIG, "r");

    if(file)
    {
        ESP_LOGI(TAG, STORAGE_PATH_CONFIG" file exist");
        /* get the file size */
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        /* file_size + 1 byte should the buffer size,
         * because string ending zero byte needed */
        char *buf = (char*)malloc(file_size + 1);

        if(buf)
        {
            fread(buf, 1, file_size, file);

            if(*buf)
            {
                buf[file_size] = 0; // string ending zero byte
                ESP_LOGI(TAG, "file content:\n%s", buf);
            }
            else ESP_LOGE(TAGE, "file empty");

            res = cJSON_Parse(buf);
            free(buf);

            if(res == NULL) ESP_LOGE(TAGE, "file content can't parsed to JSON");
        }
        else PRINT_TRACE();

        fclose(file);
    }
    else ESP_LOGE(TAGE, STORAGE_PATH_CONFIG " file NOT exist");

    if(res) return res;
    else return cJSON_CreateObject();
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

void storage_config_parse()
{
    ESP_LOGI(TAG, "read config...");
    cJSON *cfg = storage_load();
    config_parse_lights(cfg);
    cJSON_Delete(cfg);
}

static void config_parse_lights(cJSON *cfg)
{
    if(cJSON_HasObjectItem(cfg, "lights"))
    {
        ESP_LOGI(TAG, "cfg has lights");
        cJSON *cfg_lights = cJSON_GetObjectItem(cfg, "lights");

        if(cJSON_HasObjectItem(cfg_lights, "strips"))
        {
            ESP_LOGI(TAG, "cfg_lights has strips");
            cJSON *cfg_strips = cJSON_GetObjectItem(cfg_lights, "strips");

            if(cJSON_IsArray(cfg_strips))
            {
                parse_lights_strips(cfg_strips);
            }
            else ESP_LOGE(TAGE, "cfg_strips is NOT array");
        }
        else ESP_LOGE(TAGE, "cfg_lights NOT has strips");
    }
    else ESP_LOGE(TAGE, "cfg NOT has lights");
}

static void parse_lights_strips(cJSON *cfg_strips)
{
    uint8_t strip_cnt = cJSON_GetArraySize(cfg_strips);
    ESP_LOGI(TAG, "cfg_strips is %d size array", strip_cnt);

    for(uint8_t i = 0; i < strip_cnt; i++)
    {
        if(i < MLED_STRIP_N)
        {
            cJSON *cfg_strip_item = cJSON_GetArrayItem(cfg_strips, i);

            if(cfg_strip_item)
            {
                if(cJSON_HasObjectItem(cfg_strip_item, "pixel_n")
                    && cJSON_HasObjectItem(cfg_strip_item, "rgb_order"))
                {
                    ESP_LOGI(TAG, "cfg_strip_item %d has required tag", i);
                    size_t cfg_strip_pixel_n = cJSON_GetObjectItem(cfg_strip_item, "pixel_n")->valueint;
                    char *cfg_zone_colors = cJSON_GetObjectItem(cfg_strip_item, "rgb_order")->valuestring;
                    mled_rgb_order colors = {0};
                    bool valid = true;
                    char *char_p = strchr(cfg_zone_colors, 'R');

                    if(char_p) colors.i_r = char_p - cfg_zone_colors;
                    else valid = false;

                    char_p = strchr(cfg_zone_colors, 'G');

                    if(char_p) colors.i_g = char_p - cfg_zone_colors;
                    else valid = false;

                    char_p = strchr(cfg_zone_colors, 'B');

                    if(char_p) colors.i_b = char_p - cfg_zone_colors;
                    else valid = false;

                    if(valid)
                    {
                        if(colors.i_r > 2 || colors.i_g > 2 || colors.i_b > 2
                            || colors.i_r == colors.i_g || colors.i_r == colors.i_b || colors.i_g == colors.i_b)
                        {
                            valid = false;
                        }
                    }

                    if(valid)
                    {
                        lights_set_strip_size(i, cfg_strip_pixel_n);
                    }
                    else ESP_LOGE(TAGE, "cfg_zone_colors %d not valid", i);
                }
                else ESP_LOGE(TAGE, "cfg_strip_item %d NOT has all required tag", i);

                if(cJSON_HasObjectItem(cfg_strip_item, "zones"))
                {
                    ESP_LOGI(TAG, "cfg_lights has zones");
                    cJSON *cfg_zones = cJSON_GetObjectItem(cfg_strip_item, "zones");

                    if(cJSON_IsArray(cfg_zones))
                    {
                        parse_lights_zones(cfg_zones, i);
                    }
                    else ESP_LOGE(TAGE, "cfg_zones is NOT array");
                }
                else ESP_LOGE(TAGE, "cfg_lights NOT has zones");
            }
            else ESP_LOGE(TAGE, "cfg_strip_item %d is NULL", i);
        }
        else
        {
            ESP_LOGE(TAGE, "config contains more strips than max size %d/%d", strip_cnt, MLED_STRIP_N);
            break;
        }
    }
}

static void parse_lights_zones(cJSON *cfg_zones, int strip_index)
{
    uint8_t zone_cnt = cJSON_GetArraySize(cfg_zones);
    ESP_LOGI(TAG, "cfg_zones is %d size array", zone_cnt);

    for(uint8_t i = 0; i < zone_cnt; i++)
    {
        cJSON *cfg_zone_item = cJSON_GetArrayItem(cfg_zones, i);

        if(cfg_zone_item)
        {
            if(cJSON_HasObjectItem(cfg_zone_item, "pixel_n"))
            {
                ESP_LOGI(TAG, "device item %d has pixel_n", i);
                size_t cfg_zone_pixel_n = cJSON_GetObjectItem(cfg_zone_item, "pixel_n")->valueint;
                lights_new_zone(strip_index, cfg_zone_pixel_n);
            }
            else ESP_LOGE(TAGE, "cfg_zone_item %d NOT has all required tag", i);
        }
        else ESP_LOGE(TAGE, "cfg_zone_item %d is NULL", i);
    }
}
