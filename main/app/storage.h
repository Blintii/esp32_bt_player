/*
 * Non Volatile Storage handler
 */

#ifndef STORAGE_H
#define STORAGE_H


#include "cJSON.h"


#define STORAGE_PATH_CONFIG "/spiffs/config.json"


cJSON *storage_load(void);
void storage_save(cJSON *json);
void storage_config_parse();
void storage_save_lights();


#endif /* STORAGE_H */
