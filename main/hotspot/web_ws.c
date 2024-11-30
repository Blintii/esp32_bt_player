
#include <string.h>
#include "esp_log.h"

#include "app_tools.h"
#include "web.h"


static void web_ws_send_done_callback(esp_err_t err, int socketfd, void *arg);
static void web_ws_process_msg(httpd_req_t *req, httpd_ws_frame_t frame);


static const char *TAG = LOG_COLOR("96") "web_ws" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "web_ws" LOG_COLOR_E;


esp_err_t web_ws(httpd_req_t *req)
{
    if(req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "websocket handshake done, new connection: socket_%d", httpd_req_to_sockfd(req));
        // TODO: send config
        return ESP_OK;
    }

    httpd_ws_frame_t frame;
    uint8_t *buf = NULL;
    memset(&frame, 0, sizeof(httpd_ws_frame_t));
    /* max_len = 0 (3. argument of httpd_ws_recv_frame):
     *  to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    ERR_CHECK_RETURN_VAL(ret != ESP_OK, ret);

    if(frame.len)
    {
        buf = malloc(frame.len);
        ERR_IF_NULL_RETURN_VAL(buf, ESP_ERR_NO_MEM);

        frame.payload = buf;
        /* max_len = frame.len (3. argument of httpd_ws_recv_frame):
         *  to get the frame payload in struct httpd_ws_frame_t */
        ret = httpd_ws_recv_frame(req, &frame, frame.len);

        if(ret == ESP_OK) web_ws_process_msg(req, frame);
        else ESP_LOGE(TAGE, "websocket httpd_ws_recv_frame failed with %s", esp_err_to_name(ret));
    }

    free(buf);
    return ret;
}

void web_ws_send(int sockfd, uint8_t *payload, size_t len)
{
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(httpd_ws_frame_t));
    frame.payload = malloc(len);
    ERR_IF_NULL_RETURN(frame.payload);
    memcpy(frame.payload, payload, len);
    frame.len = len;
    frame.type = HTTPD_WS_TYPE_BINARY;

    printf("WS_TX_%d[%d] = ", sockfd, len);
    PRINT_ARRAY_HEX(payload, len);

    httpd_ws_send_data_async(web_server_hd, sockfd, &frame, web_ws_send_done_callback, frame.payload);
}

void web_ws_send_all(uint8_t *payload, size_t len)
{
    int sockets[HTTPD_MAX_SOCKETS] = {0};
    size_t cnt = HTTPD_MAX_SOCKETS;

    if(ESP_OK == httpd_get_client_list(web_server_hd, &cnt, sockets))
    {
        for(size_t i = 0; i < cnt; i++)
        {
            if(HTTPD_WS_CLIENT_WEBSOCKET == httpd_ws_get_fd_info(web_server_hd, sockets[i]))
            {
                web_ws_send(sockets[i], payload, len);
            }
        }
    }
}

static void web_ws_send_done_callback(esp_err_t err, int socketfd, void *arg)
{
    /* arg is the frame payload cames from web_ws_send() */
    if(err != ESP_OK)
    {
        ESP_LOGE(TAGE, "send done callback socket_%d error: %s", socketfd, esp_err_to_name(err));
    }

    free(arg);
}

static void web_ws_process_msg(httpd_req_t *req, httpd_ws_frame_t frame)
{
    printf("WS_RX_%d[%d] = ", httpd_req_to_sockfd(req), frame.len);
    PRINT_ARRAY_HEX(frame.payload, frame.len);

    switch(frame.payload[0])
    {
        default: ESP_LOGE(TAGE, "unknown WS message");
    }
}
