
#include "stdio.h"
#include "string.h"
#include "esp_system.h"
#include "esp_log.h"

#include "app_tools.h"
#include "app_config.h"
#include "file_system.h"
#include "storage.h"
#include "lights.h"
#include "tasks.h"


static const char *TAG = LOG_COLOR("33") "cfg" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("33") "cfg" LOG_COLOR_E;


static void config_parse_lights(cJSON *cfg);
static void parse_lights_strips(cJSON *cfg_strips);
static void parse_lights_zones(cJSON *cfg_zones, int strip_index);
static void parse_lights_shader(cJSON *cfg_shader, lights_shader *shader);
static void parse_lights_shader_single(cJSON *cfg_shader, lights_shader *shader);
static void parse_lights_shader_repeat(cJSON *cfg_shader, lights_shader *shader);
static void parse_lights_shader_fade(cJSON *cfg_shader, lights_shader *shader);
static void parse_lights_shader_fft(cJSON *cfg_shader, lights_shader *shader);
static int parse_lights_colors_rgb(cJSON *cfg_colors, color_rgb **colors);
static int parse_lights_colors_hsl(cJSON *cfg_colors, color_hsl **colors);
static void config_update_lights(cJSON *cfg_lights);


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
    cJSON *cfg = storage_load();
    config_parse_lights(cfg);
    cJSON_Delete(cfg);
}

void storage_save_lights()
{
    cJSON *cfg = storage_load();
    cJSON *cfg_lights;

    if(cJSON_HasObjectItem(cfg, "lights"))
    {
        ESP_LOGI(TAG, "cfg has lights");
        /* force clean whole existing lights config */
        cJSON_DeleteItemFromObject(cfg, "lights");
    }
    else ESP_LOGI(TAG, "cfg NOT has lights");

    cfg_lights = cJSON_CreateObject();
    cJSON_AddItemToObject(cfg, "lights", cfg_lights);
    config_update_lights(cfg_lights);
    storage_save(cfg);
    cJSON_Delete(cfg);
}

static void config_parse_lights(cJSON *cfg)
{
    ESP_LOGI(TAG, "json parse lights start...");

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
                if(tasks_lights_lock())
                {
                    parse_lights_strips(cfg_strips);
                    tasks_lights_release();
                }
            }
            else ESP_LOGE(TAGE, "cfg_strips is NOT array");
        }
        else ESP_LOGE(TAGE, "cfg_lights NOT has strips");
    }
    else ESP_LOGE(TAGE, "cfg NOT has lights");

    ESP_LOGI(TAG, "json parse lights end");
}

static void parse_lights_strips(cJSON *cfg_strips)
{
    int strip_cnt = cJSON_GetArraySize(cfg_strips);
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
                    int cfg_strip_pixel_n = cJSON_GetObjectItem(cfg_strip_item, "pixel_n")->valueint;
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

                        if(cJSON_HasObjectItem(cfg_strip_item, "zones"))
                        {
                            ESP_LOGI(TAG, "cfg_lights %d has zones", i);
                            cJSON *cfg_zones = cJSON_GetObjectItem(cfg_strip_item, "zones");

                            if(cJSON_IsArray(cfg_zones))
                            {
                                parse_lights_zones(cfg_zones, i);
                            }
                            else ESP_LOGE(TAGE, "cfg_zones %d is NOT array", i);
                        }
                        else ESP_LOGE(TAGE, "cfg_lights %d NOT has zones", i);
                    }
                    else ESP_LOGE(TAGE, "cfg_zone_colors %d not valid", i);
                }
                else ESP_LOGE(TAGE, "cfg_strip_item %d NOT has all required tag", i);
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
    int zone_cnt = cJSON_GetArraySize(cfg_zones);
    ESP_LOGI(TAG, "cfg_zones is %d size array", zone_cnt);

    for(uint8_t i = 0; i < zone_cnt; i++)
    {
        cJSON *cfg_zone_item = cJSON_GetArrayItem(cfg_zones, i);

        if(cfg_zone_item)
        {
            if(cJSON_HasObjectItem(cfg_zone_item, "pixel_n"))
            {
                ESP_LOGI(TAG, "cfg_zone_item %d has pixel_n", i);
                int cfg_zone_pixel_n = cJSON_GetObjectItem(cfg_zone_item, "pixel_n")->valueint;
                lights_zone_chain *zone = lights_new_zone(strip_index, cfg_zone_pixel_n);

                if(!zone) continue;

                if(cJSON_HasObjectItem(cfg_zone_item, "shader"))
                {
                    ESP_LOGI(TAG, "cfg_zone_item %d has shader", i);
                    cJSON *cfg_shader = cJSON_GetObjectItem(cfg_zone_item, "shader");
                    parse_lights_shader(cfg_shader, &zone->shader);
                    zone->shader.need_render = true;
                }
                else ESP_LOGE(TAGE, "cfg_zone_item %d NOT has shader", i);
            }
            else ESP_LOGE(TAGE, "cfg_zone_item %d NOT has pixel_n", i);
        }
        else ESP_LOGE(TAGE, "cfg_zone_item %d is NULL", i);
    }
}

static void parse_lights_shader(cJSON *cfg_shader, lights_shader *shader)
{
    if(cJSON_HasObjectItem(cfg_shader, "type"))
    {
        ESP_LOGI(TAG, "cfg_shader has type");
        int cfg_shader_type = cJSON_GetObjectItem(cfg_shader, "type")->valueint;

        switch(cfg_shader_type)
        {
            case SHADER_SINGLE: parse_lights_shader_single(cfg_shader, shader); break;
            case SHADER_FADE: parse_lights_shader_repeat(cfg_shader, shader); break;
            case SHADER_REPEAT: parse_lights_shader_fade(cfg_shader, shader); break;
            case SHADER_FFT: parse_lights_shader_fft(cfg_shader, shader); break;
            default: ERR_BAD_CASE(cfg_shader_type, "%d");
        }
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has type");
}

static void parse_lights_shader_single(cJSON *cfg_shader, lights_shader *shader)
{
    if(cJSON_HasObjectItem(cfg_shader, "color"))
    {
        cJSON *cfg_color = cJSON_GetObjectItem(cfg_shader, "color");

        if(cJSON_HasObjectItem(cfg_color, "r")
            && cJSON_HasObjectItem(cfg_color, "g")
            && cJSON_HasObjectItem(cfg_color, "b"))
        {
            ESP_LOGI(TAG, "cfg_color has rgb");
            int cfg_r = cJSON_GetObjectItem(cfg_color, "r")->valueint;
            int cfg_g = cJSON_GetObjectItem(cfg_color, "g")->valueint;
            int cfg_b = cJSON_GetObjectItem(cfg_color, "b")->valueint;
            shader->cfg.shader_single.color = (color_rgb) {cfg_r, cfg_g, cfg_b};
        }
        else ESP_LOGE(TAGE, "cfg_color NOT has rgb");
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has color");
}

static void parse_lights_shader_repeat(cJSON *cfg_shader, lights_shader *shader)
{
    lights_shader_cfg_repeat *shader_repeat = &shader->cfg.shader_repeat;

    if(cJSON_HasObjectItem(cfg_shader, "colors"))
    {
        cJSON *cfg_colors = cJSON_GetObjectItem(cfg_shader, "colors");
        shader_repeat->color_n = parse_lights_colors_rgb(cfg_colors, &shader_repeat->colors);
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has colors");
}

static void parse_lights_shader_fade(cJSON *cfg_shader, lights_shader *shader)
{
    lights_shader_cfg_fade *shader_fade = &shader->cfg.shader_fade;

    if(cJSON_HasObjectItem(cfg_shader, "colors"))
    {
        cJSON *cfg_colors = cJSON_GetObjectItem(cfg_shader, "colors");
        shader_fade->color_n = parse_lights_colors_hsl(cfg_colors, &shader_fade->colors);
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has colors");
}

static void parse_lights_shader_fft(cJSON *cfg_shader, lights_shader *shader)
{
    lights_shader_cfg_fft *shader_fft = &shader->cfg.shader_fft;

    if(cJSON_HasObjectItem(cfg_shader, "colors"))
    {
        cJSON *cfg_colors = cJSON_GetObjectItem(cfg_shader, "colors");
        shader_fft->color_n = parse_lights_colors_hsl(cfg_colors, &shader_fft->colors);
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has colors");

    if(cJSON_HasObjectItem(cfg_shader, "is_right"))
    {
        int cfg_is_right = cJSON_GetObjectItem(cfg_shader, "is_right")->valueint;
        shader_fft->is_right = cfg_is_right;
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has is_right");

    if(cJSON_HasObjectItem(cfg_shader, "intensity"))
    {
        int cfg_intensity = cJSON_GetObjectItem(cfg_shader, "intensity")->valuedouble;
        shader_fft->intensity = cfg_intensity;
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has intensity");

    if(cJSON_HasObjectItem(cfg_shader, "mirror"))
    {
        int cfg_mirror = cJSON_GetObjectItem(cfg_shader, "mirror")->valueint;
        shader_fft->mirror = cfg_mirror;
    }
    else ESP_LOGE(TAGE, "cfg_shader NOT has mirror");
}

static int parse_lights_colors_rgb(cJSON *cfg_colors, color_rgb **colors)
{
    int color_n = 0;

    if(cJSON_IsArray(cfg_colors))
    {
        color_n = cJSON_GetArraySize(cfg_colors);
        color_rgb *color_array = (color_rgb*) calloc(color_n, sizeof(color_rgb));
        ERR_IF_NULL_RETURN_VAL(color_array, 0);
        *colors = color_array;

        for(uint8_t i = 0; i < color_n; i++)
        {
            cJSON *cfg_color_item = cJSON_GetArrayItem(cfg_colors, i);

            if(cfg_color_item)
            {
                if(cJSON_HasObjectItem(cfg_color_item, "r")
                    && cJSON_HasObjectItem(cfg_color_item, "g")
                    && cJSON_HasObjectItem(cfg_color_item, "b"))
                {
                    ESP_LOGI(TAG, "cfg_color_item %d has rgb", i);
                    int cfg_r = cJSON_GetObjectItem(cfg_color_item, "r")->valueint;
                    int cfg_g = cJSON_GetObjectItem(cfg_color_item, "g")->valueint;
                    int cfg_b = cJSON_GetObjectItem(cfg_color_item, "b")->valueint;
                    color_array[i] = (color_rgb) {cfg_r, cfg_g, cfg_b};
                }
                else ESP_LOGE(TAGE, "cfg_color_item %d NOT has rgb", i);
            }
            else ESP_LOGE(TAGE, "cfg_color_item %d is NULL", i);
        }
    }
    else ESP_LOGE(TAGE, "cfg_colors is NOT array");

    return color_n;
}

static int parse_lights_colors_hsl(cJSON *cfg_colors, color_hsl **colors)
{
    int color_n = 0;

    if(cJSON_IsArray(cfg_colors))
    {
        color_n = cJSON_GetArraySize(cfg_colors);
        color_hsl *color_array = (color_hsl*) calloc(color_n, sizeof(color_hsl));
        ERR_IF_NULL_RETURN_VAL(color_array, 0);
        *colors = color_array;

        for(uint8_t i = 0; i < color_n; i++)
        {
            cJSON *cfg_color_item = cJSON_GetArrayItem(cfg_colors, i);

            if(cfg_color_item)
            {
                if(cJSON_HasObjectItem(cfg_color_item, "r")
                    && cJSON_HasObjectItem(cfg_color_item, "g")
                    && cJSON_HasObjectItem(cfg_color_item, "b"))
                {
                    ESP_LOGI(TAG, "cfg_color_item %d has hsl", i);
                    double cfg_hue = cJSON_GetObjectItem(cfg_color_item, "hue")->valuedouble;
                    double cfg_sat = cJSON_GetObjectItem(cfg_color_item, "sat")->valuedouble;
                    double cfg_lum = cJSON_GetObjectItem(cfg_color_item, "lum")->valuedouble;
                    color_array[i] = (color_hsl) {cfg_hue, cfg_sat, cfg_lum};
                }
                else ESP_LOGE(TAGE, "cfg_color_item %d NOT has hsl", i);
            }
            else ESP_LOGE(TAGE, "cfg_color_item %d is NULL", i);
        }
    }
    else ESP_LOGE(TAGE, "cfg_colors is NOT array");

    return color_n;
}

static void config_update_lights(cJSON *cfg_lights)
{
    cJSON *cfg_strips = cJSON_CreateArray();
    cJSON_AddItemToObject(cfg_lights, "strips", cfg_strips);
    /* RGB + zero ending */
    char rgb_order_str[4] = {0};

    for(uint8_t strip_index = 0; strip_index < MLED_STRIP_N; strip_index++)
    {
        mled_strip *strip = &mled_channels[strip_index];
        cJSON *cfg_strip_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(cfg_strip_item, "pixel_n", strip->pixels.pixel_n);
        mled_rgb_order rgb_order = strip->rgb_order;
        rgb_order_str[rgb_order.i_r] = 'R';
        rgb_order_str[rgb_order.i_g] = 'G';
        rgb_order_str[rgb_order.i_b] = 'B';
        cJSON_AddStringToObject(cfg_strip_item, "rgb_order", rgb_order_str);
        lights_zone_chain *zone = lights_zones[strip_index].first;

        while(zone)
        {
            cJSON *cfg_zone_item = cJSON_CreateObject();
            cJSON_AddNumberToObject(cfg_zone_item, "pixel_n", zone->frame_buf.pixel_n);
            lights_shader *shader = &zone->shader;
            cJSON *cfg_shader = cJSON_CreateObject();
            cJSON_AddNumberToObject(cfg_zone_item, "type", shader->type);

            switch(shader->type)
            {
                case SHADER_SINGLE: {
                    color_rgb color = shader->cfg.shader_single.color;
                    cJSON *cfg_color = cJSON_CreateObject();
                    cJSON_AddNumberToObject(cfg_color, "r", color.r);
                    cJSON_AddNumberToObject(cfg_color, "g", color.g);
                    cJSON_AddNumberToObject(cfg_color, "b", color.b);
                    cJSON_AddItemToObject(cfg_shader, "color", cfg_color);
                    break;
                }
                // TODO other shaders
                // case SHADER_REPEAT: break;
                // case SHADER_FADE: break;
                // case SHADER_FFT: break;
                default: ERR_BAD_CASE(shader->type, "%d");
            }

            cJSON_AddItemToObject(cfg_zone_item, "shader", cfg_shader);
            cJSON_AddItemToArray(cfg_strip_item, cfg_zone_item);
            zone = zone->next;
        }

        cJSON_AddItemToArray(cfg_strips, cfg_strip_item);
    }
}
