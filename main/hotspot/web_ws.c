
#include <string.h>
#include "esp_log.h"

#include "app_tools.h"
#include "app_config.h"
#include "web.h"
#include "lights.h"


static void web_ws_send_done_callback(esp_err_t err, int socketfd, void *arg);
static void web_ws_process_msg(httpd_req_t *req, httpd_ws_frame_t frame);
static void web_ws_send_strips(int sockfd);
static void web_ws_send_zones(int sockfd);
static void web_ws_send_shader(lights_zone_chain *zone, uint8_t strip_index, uint8_t zone_index, int sockfd);


static const char *TAG = LOG_COLOR("96") "web_ws" LOG_RESET_COLOR;
static const char *TAGE = LOG_COLOR("96") "web_ws" LOG_COLOR_E;


esp_err_t web_ws(httpd_req_t *req)
{
    if(req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "websocket handshake done, new connection: socket_%d", httpd_req_to_sockfd(req));
        int sockfd = httpd_req_to_sockfd(req);
        web_ws_send_strips(sockfd);
        web_ws_send_zones(sockfd);

        for(uint8_t strip_index = 0; strip_index < MLED_STRIP_N; strip_index++)
        {
            lights_zone_list *list = &lights_zones[strip_index];
            lights_zone_chain *zone = list->first;
            uint8_t zone_index = 0;

            while(zone)
            {
                web_ws_send_shader(zone, strip_index, zone_index, sockfd);
                zone = zone->next;
                zone_index++;
            }
        }

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
    httpd_ws_frame_t frame = {
        .payload = payload,
        .len = len,
        .type = HTTPD_WS_TYPE_BINARY
    };

    printf("WS_TX_%d[%d] = ", sockfd, len);
    PRINT_ARRAY_HEX(payload, len);

    httpd_ws_send_data_async(web_server_hd, sockfd, &frame, web_ws_send_done_callback, frame.payload);
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
        // case WEB_WS_SID_STRIP_SET:
        //     break;
        // case WEB_WS_SID_ZONE_SET:
        //     break;
        // case WEB_WS_SID_SHADER_SET:
        //     break;
        default: ESP_LOGE(TAGE, "unknown WS message");
    }
}

static void web_ws_send_strips(int sockfd)
{
    /* CID + strip_n * (pixel_n + rgb_order) */
    size_t len = 1 + MLED_STRIP_N * (sizeof(size_t) + 3);
    uint8_t *payload = (uint8_t*)calloc(1, len);
    ERR_IF_NULL_RETURN(payload);
    uint8_t *p = payload;
    *p++ = WEB_WS_CID_STRIP_CFG;
    size_t *p_size;
    mled_rgb_order rgb_order;

    for(uint8_t i = 0; i < MLED_STRIP_N; i++)
    {
        p_size = (size_t*)p;
        *p_size++ = mled_channels[i].pixels.pixel_n;
        p = (uint8_t*)p_size;
        rgb_order = mled_channels[i].rgb_order;
        p[rgb_order.i_r] = 'R';
        p[rgb_order.i_g] = 'G';
        p[rgb_order.i_b] = 'B';
        p += 3;
    }

    ESP_LOGW(TAG, "send strips payload bytes: %d (check: %d)", len, (p - payload));
    web_ws_send(sockfd, payload, len);
}

static void web_ws_send_zones(int sockfd)
{
    for(uint8_t i = 0; i < MLED_STRIP_N; i++)
    {
        lights_zone_list *list = &lights_zones[i];

        if(!list->zone_n) continue;

        /* CID + strip index + zone_n * pixel_n */
        size_t len = 1 + 1 + list->zone_n * sizeof(size_t);
        uint8_t *payload = (uint8_t*)calloc(1, len);
        ERR_IF_NULL_RETURN(payload);
        uint8_t *p = payload;
        *p++ = WEB_WS_CID_ZONE_CFG;
        *p++ = i;
        size_t *p_size = (size_t*)p;
        lights_zone_chain *zone = list->first;

        while(zone)
        {
            *p_size++ = zone->frame_buf.pixel_n;
            zone = zone->next;
        }

        p = (uint8_t*)p_size;
        ESP_LOGW(TAG, "send zones payload bytes: %d (check: %d)", len, (p - payload));
        web_ws_send(sockfd, payload, len);
    }
}

static void web_ws_send_shader(lights_zone_chain *zone, uint8_t strip_index, uint8_t zone_index, int sockfd)
{
    lights_shader *shader = &zone->shader;
    /* CID + strip index + zone index * shader type */
    size_t len = 1 + 1 + 1 + 1;

    switch(shader->type)
    {
        case SHADER_SINGLE:
            len += 3;
            break;
        case SHADER_REPEAT:
            len += shader->cfg.shader_repeat.color_n * sizeof(color_rgb);
            break;
        case SHADER_FADE:
            len += shader->cfg.shader_fade.color_n * sizeof(color_hsl);
            break;
        case SHADER_FFT:
            len += shader->cfg.shader_fft.color_n * sizeof(color_hsl);
            /* is right + intensity + mirror */
            len += 1 + sizeof(float) + 1;
            break;
        default:
            ERR_BAD_CASE(shader->type, "%d");
    }

    uint8_t *payload = (uint8_t*)calloc(1, len);
    ERR_IF_NULL_RETURN(payload);
    uint8_t *p = payload;
    *p++ = WEB_WS_CID_SHADER_CFG;
    *p++ = strip_index;
    *p++ = zone_index;
    *p++ = shader->type;

    switch(shader->type)
    {
        case SHADER_SINGLE: {
            color_rgb color = shader->cfg.shader_single.color;
            *p++ = color.r;
            *p++ = color.g;
            *p++ = color.b;
            break;
        }
        case SHADER_REPEAT: {
            lights_shader_cfg_repeat *cfg = &shader->cfg.shader_repeat;

            for(uint8_t i = 0; i < cfg->color_n; i++)
            {
                *p++ = cfg->colors[i].r;
                *p++ = cfg->colors[i].g;
                *p++ = cfg->colors[i].b;
            }
            break;
        }
        case SHADER_FADE: {
            lights_shader_cfg_fade *cfg = &shader->cfg.shader_fade;
            float *p_float = (float*)p;

            for(uint8_t i = 0; i < cfg->color_n; i++)
            {
                *p_float++ = cfg->colors[i].hue;
                *p_float++ = cfg->colors[i].sat;
                *p_float++ = cfg->colors[i].lum;
            }

            p = (uint8_t*)p_float;
            break;
        }
        case SHADER_FFT: {
            lights_shader_cfg_fft *cfg = &shader->cfg.shader_fft;
            float *p_float = (float*)p;

            for(uint8_t i = 0; i < cfg->color_n; i++)
            {
                *p_float++ = cfg->colors[i].hue;
                *p_float++ = cfg->colors[i].sat;
                *p_float++ = cfg->colors[i].lum;
            }

            p = (uint8_t*)p_float;
            *p++ = cfg->is_right;
            p_float = (float*)p;
            *p_float++ = cfg->intensity;
            p = (uint8_t*)p_float;
            *p++ = cfg->mirror;
            break;
        }
        default:
            ERR_BAD_CASE(shader->type, "%d");
    }

    ESP_LOGW(TAG, "send zones shader bytes: %d (check: %d)", len, (p - payload));
    web_ws_send(sockfd, payload, len);
}
