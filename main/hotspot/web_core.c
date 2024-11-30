/* Created: 2024.07.19.
 * By: Blinti
 */

#include "esp_log.h"
#include "socket.h"

#include "app_tools.h"
#include "web.h"


static const char *URLredirect = "http://audioreact.led/";
static const char *TAG = LOG_COLOR("96") "web" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "web" LOG_COLOR_E;
httpd_handle_t web_server_hd = NULL;


/* Function to start the file server */
esp_err_t web_start_server()
{
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

    ESP_LOGI(TAG, "HTTP server starting...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 1;
    config.stack_size = 3136;
    config.lru_purge_enable = true;
    config.max_open_sockets = HTTPD_MAX_SOCKETS;
    config.recv_wait_timeout = 1;
    config.send_wait_timeout = 1;
    config.backlog_conn = 3;
    config.max_resp_headers = 1;
    config.max_uri_handlers = 3;

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;
    esp_err_t err = httpd_start(&web_server_hd, &config);

    if(ESP_OK != err)
    {
        ESP_LOGE(TAGE, "failed to start file server: %s", esp_err_to_name(err));
        return err;
    }
    else ESP_LOGI(TAG, "HTTP server started on port: '%d'", config.server_port);

    /* URI handler for getting files */
    httpd_uri_t ws_handler = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = web_ws,
        .is_websocket = true
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(web_server_hd, &ws_handler));

    httpd_uri_t unsafe_content_handler = {
        .uri = "/spiffs/*",
        .method = HTTP_GET,
        .handler = web_unsafe_file_content,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(web_server_hd, &unsafe_content_handler));

    httpd_uri_t content_handler = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = web_file_content
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(web_server_hd, &content_handler));

    ESP_ERROR_CHECK(httpd_register_err_handler(web_server_hd, HTTPD_405_METHOD_NOT_ALLOWED, web_redirect));
    ESP_LOGI(TAG, "HTTP server started OK");
    return ESP_OK;
}

esp_err_t web_redirect(httpd_req_t *req, httpd_err_code_t err)
{
    // ESP_LOGI(TAG, "%sRedirect %s to %s", LOG_COLOR(LOG_COLOR_BROWN), req->uri, URLredirect);
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", URLredirect);
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}
