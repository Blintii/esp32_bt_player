/*
 * Application logging helper tools
 */

#ifndef __APP_TOOLS_H__
#define __APP_TOOLS_H__


#include "esp_log.h"
#include "esp_debug_helpers.h"


#define ERR_IF_NULL(x) do {                            \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            PRINT_TRACE();                             \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RETURN(x) do {                     \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            PRINT_TRACE();                             \
            return;                                    \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RETURN_VAL(x, val) do {            \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            PRINT_TRACE();                             \
            return val;                                \
        }                                              \
    } while(0)

#define ERR_IF_NULL_RESET(x) do {                      \
        if(x == NULL) {                                \
            ESP_LOGE(TAGE, "NULL in %s", __func__);    \
            PRINT_TRACE();                             \
            abort();                                   \
        }                                              \
    } while(0)

#define ERR_CHECK(x) do {                              \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            PRINT_TRACE();                             \
        }                                              \
    } while(0)

#define ERR_CHECK_RETURN(x) do {                       \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            PRINT_TRACE();                             \
            return;                                    \
        }                                              \
    } while(0)

#define ERR_CHECK_RETURN_VAL(x, val) do {              \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            PRINT_TRACE();                             \
            return val;                                \
        }                                              \
    } while(0)

#define ERR_CHECK_RESET(x) do {                        \
        if(x) {                                        \
            ESP_LOGE(TAGE, "ERROR in %s", __func__);   \
            PRINT_TRACE();                             \
            abort();                                   \
        }                                              \
    } while(0)

#define ERR_BAD_CASE(case, format) do {                                     \
        ESP_LOGE(TAGE, "unhanled case: "format", in %s", case, __func__);   \
        PRINT_TRACE();                                                      \
    } while(0)

#define PRINT_ARRAY_HEX(array, len) do {    \
        for(uint8_t i = 0; i < len; i++)    \
        {                                   \
            printf(" %02X", array[i]);      \
        }                                   \
        printf("\n");                       \
    } while(0)

#define PRINT_TRACE() esp_backtrace_print(4)


void list_tasks_stack_info();


#endif /* __APP_TOOLS_H__ */
