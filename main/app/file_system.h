/*
 * SPIFFS file system handler
 */

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H


#include "esp_vfs.h"


#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)


esp_err_t spiffs_init(void);


#endif /* FILE_SYSTEM_H */
