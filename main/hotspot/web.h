/*
 * Webserver (file + websocket) handler
 */

#ifndef WEB_H
#define WEB_H


#include "esp_http_server.h"
#include "esp_vfs.h"


/* path to web files location in the flash storage
 * files have to flashed to the spiffs storage with CMake spiffs_create_partition_image */
#define WEB_FILE_PATH "/spiffs/web"

/* scratch buffer size for temporary storage during file transfer*/
#define SCRATCH_BUFSIZE 256

/* used to check filename has the given extension ending or not*/
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* number of max opened sockets in the webserver */
#define HTTPD_MAX_SOCKETS (CONFIG_LWIP_MAX_SOCKETS - 6)


typedef enum {
    WEB_WS_CID_STRIP_CFG,
    WEB_WS_CID_ZONE_CFG,
    WEB_WS_CID_SHADER_CFG,
} web_ws_id_clientbound;

typedef enum {
    WEB_WS_SID_STRIP_SET,
    WEB_WS_SID_ZONE_SET,
    WEB_WS_SID_SHADER_SET,
} web_ws_id_serverbound;


void web_server_start();
void web_server_stop();
esp_err_t web_redirect(httpd_req_t *req, httpd_err_code_t err);
esp_err_t web_file_content(httpd_req_t *req);
esp_err_t web_unsafe_file_content(httpd_req_t *req);
esp_err_t web_ws(httpd_req_t *req);
void web_ws_send(int sockfd, uint8_t *payload, size_t len);
void web_ws_send_all(uint8_t *payload, size_t len);


extern httpd_handle_t web_server_hd;


#endif /* WEB_H */
