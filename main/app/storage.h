/*
 * Non Volatile Storage handler
 */

#ifndef STORAGE_H
#define STORAGE_H


#include "cJSON.h"


#define STORAGE_PATH_CONFIG "/spiffs/config.json"
#define STORAGE_FILE_SIZE 1536


cJSON *storage_load(void);
void storage_save(cJSON *json);


#endif /* STORAGE_H */
