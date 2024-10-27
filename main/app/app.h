/* Created: 2024.10.08.
 * By: Blinti
 */

#ifndef __APP_H__
#define __APP_H__


#include "esp_log.h"
#include "esp_debug_helpers.h"


#define ERR_IF_NULL(x) do {                            \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            esp_backtrace_print(4);                    \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RETURN(x) do {                     \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            esp_backtrace_print(4);                    \
            return;                                    \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RETURN_VAL(x, val) do {            \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            esp_backtrace_print(4);                    \
            return val;                                \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RESET(x) do {                      \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            esp_backtrace_print(4);                    \
            abort();                                   \
        }                                              \
    } while(0)

#define ERR_CHECK(x) do {                              \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            esp_backtrace_print(4);                    \
        }                                              \
    } while(0)

#define ERR_CHECK_RETURN(x) do {                       \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            esp_backtrace_print(4);                    \
            return;                                    \
        }                                              \
    } while(0)

#define ERR_CHECK_RETURN_VAL(x, val) do {              \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            esp_backtrace_print(4);                    \
            return val;                                \
        }                                              \
    } while(0)

#define ERR_CHECK_RESET(x) do {                        \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            esp_backtrace_print(4);                    \
            abort();                                   \
        }                                              \
    } while(0)

#define PRINT_ARRAY_HEX(array, len) do {    \
        for(uint8_t i = 0; i < len; i++)    \
        {                                   \
            printf(" %02X", array[i]);      \
        }                                   \
        printf("\n");                       \
    } while(0)

void list_tasks_stack_info();

#endif /* __APP_H__ */
