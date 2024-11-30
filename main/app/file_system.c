
#include <stdio.h>
#include <dirent.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

#include "app_tools.h"
#include "file_system.h"


static const char *TAG = LOG_COLOR("90") "file";
static const char *TAGE = LOG_COLOR("90") "file" LOG_COLOR_E;


static esp_err_t list_files(const char *dirpath)
{
    char entrypath[FILE_PATH_MAX];
    char entrysize[16];
    const char *entrytype;

    struct dirent *entry;
    struct stat entry_stat;

    DIR *dir = opendir(dirpath);
    const size_t dirpath_len = strlen(dirpath);

    /* Retrieve the base path of file storage to construct the full path */
    strlcpy(entrypath, dirpath, FILE_PATH_MAX);

    if(!dir)
    {
        ESP_LOGE(TAGE, "Failed to open dir : %s", dirpath);
        return ESP_FAIL;
    }

    /* Iterate over all files / folders and fetch their names and sizes */
    while((entry = readdir(dir)) != NULL)
    {
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");
        strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);

        if(stat(entrypath, &entry_stat) == -1)
        {
            ESP_LOGE(TAGE, "Failed to stat %s : %s", entrytype, entry->d_name);
            continue;
        }

        snprintf(entrysize, 15, "%ld", entry_stat.st_size);
        ESP_LOGI(TAG, "Found %s: " LOG_COLOR("32") "%s (%s bytes)", entrytype, entry->d_name, entrysize);
    }

    closedir(dir);
    return ESP_OK;
}

/* Function to initialize SPIFFS */
esp_err_t spiffs_init(void)
{
    ESP_LOGI(TAG, "init SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAGE, "failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAGE, "failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAGE, "failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);

    if (ret != ESP_OK) {
        ESP_LOGE(TAGE, "failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    float percent = (used * 100);
    percent /= (float)total;
    ESP_LOGI(TAG, LOG_RESET_COLOR "partition size: total: %d, used: %d (%d%%)", total, used, (int)percent);
    list_files("/spiffs/");
    return ESP_OK;
}
