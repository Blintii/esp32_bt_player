
#include <sys/param.h>
#include "esp_log.h"

#include "app_tools.h"
#include "web.h"
#include "file_system.h"


static esp_err_t web_set_content_type(httpd_req_t *req, const char *filename);
static const char* web_get_filepath(char *dest, const char *uri);
static const char* web_unsafe_get_filename(const char *uri);
static esp_err_t web_serve_file(httpd_req_t *req, const char *filename, const char *filepath);


static const char *URLmainPage = "/index.html";
static const char *TAG = LOG_COLOR("36") "web_fs";
static const char *TAGE = LOG_COLOR("36") "web_fs" LOG_COLOR_E;


esp_err_t web_file_content(httpd_req_t *req)
{
    if(strcmp(req->uri, "/"))
    {
        /* URI != "/"
         * if strings NOT equal
         * serve files from file system */
        char filepath[FILE_PATH_MAX];
        const char *filename = web_get_filepath(filepath, req->uri);
        return web_serve_file(req, filename, filepath);
    }
    else
    {
        /* URI == "/"
         * 0, if strings equal
         * serve main page from specific path */
        char filepath[FILE_PATH_MAX];
        const size_t base_pathlen = strlen(WEB_FILE_PATH);
        strcpy(filepath, WEB_FILE_PATH);
        strcpy(filepath + base_pathlen, URLmainPage);

        return web_serve_file(req, filepath + base_pathlen, filepath);
    }
}

esp_err_t web_unsafe_file_content(httpd_req_t *req)
{
    const char *filename = web_unsafe_get_filename(req->uri);
    return web_serve_file(req, filename, req->uri);
}

/* set HTTP response content type according to file extension */
static esp_err_t web_set_content_type(httpd_req_t *req, const char *filename)
{
    if(IS_FILE_EXT(filename,      ".html")) return httpd_resp_set_type(req,        "text/html");
    else if(IS_FILE_EXT(filename,  ".css")) return httpd_resp_set_type(req,        "text/css");
    else if(IS_FILE_EXT(filename,   ".js")) return httpd_resp_set_type(req, "application/javascript");
    else if(IS_FILE_EXT(filename,  ".ico")) return httpd_resp_set_type(req,       "image/x-icon");
    else if(IS_FILE_EXT(filename,  ".gif")) return httpd_resp_set_type(req,       "image/gif");
    else if(IS_FILE_EXT(filename,  ".png")) return httpd_resp_set_type(req,       "image/png");
    else if(IS_FILE_EXT(filename,  ".jpg")) return httpd_resp_set_type(req,       "image/jpeg");
    else if(IS_FILE_EXT(filename, ".jpeg")) return httpd_resp_set_type(req,       "image/jpeg");
    else if(IS_FILE_EXT(filename,  ".xml")) return httpd_resp_set_type(req,        "text/xml");
    else if(IS_FILE_EXT(filename,  ".pdf")) return httpd_resp_set_type(req, "application/x-pdf");
    else if(IS_FILE_EXT(filename,  ".zip")) return httpd_resp_set_type(req, "application/x-zip");
    else if(IS_FILE_EXT(filename,   ".gz")) return httpd_resp_set_type(req, "application/x-gzip");

    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* web_get_filepath(char *dest, const char *uri)
{
    const size_t base_pathlen = strlen(WEB_FILE_PATH);
    size_t pathlen = strlen(uri);

    /* strchr: refer to first occurence of a character from a given string */
    const char *quest = strchr(uri, '?');
    const char *hash = strchr(uri, '#');
    /* if '?' or '#' exist, reduce the path len to skip they */
    if(quest) pathlen = MIN(pathlen, quest - uri);
    if(hash) pathlen = MIN(pathlen, hash - uri);

    if(base_pathlen + pathlen + 1 > FILE_PATH_MAX)
    {
        /* full path string won't fit into destination buffer */
        return NULL;
    }

    /* construct full path (base + path) */
    strcpy(dest, WEB_FILE_PATH);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* return pointer to path, skipping the base */
    return dest + base_pathlen;
}

static const char* web_unsafe_get_filename(const char *uri)
{
    /* strchr: refer to last occurence of a character from a given string */
    const char *sep = strrchr(uri, '/');
    return sep + 1;
}

/* Handler to download a file kept on the server */
static esp_err_t web_serve_file(httpd_req_t *req, const char *filename, const char *filepath)
{
    //ESP_LOGI(TAG, "%sFile request: %s", LOG_RESET_COLOR, req->uri);
    struct stat file_stat;

    if(!filename)
    {
        ESP_LOGE(TAGE, "filename too long: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, NULL);
        return ESP_FAIL;
    }

    if(stat(filepath, &file_stat) == -1)
    {
        // ESP_LOGI(TAG, "%sFile not found: %s", LOG_COLOR(LOG_COLOR_BROWN), filepath);
        return web_redirect(req, HTTPD_404_NOT_FOUND);
    }

    FILE *file = fopen(filepath, "r");

    if(!file)
    {
        ESP_LOGE(TAGE, "failed to read existing file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to read existing file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "file sending: %s (%ld bytes)...", filepath, file_stat.st_size);
    web_set_content_type(req, filename);
    char *chunk = (char*) malloc(SCRATCH_BUFSIZE);

    if(!chunk)
    {
        PRINT_TRACE();
        fclose(file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory");
        return ESP_FAIL;
    }

    size_t chunksize;

    do
    {
        /* read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, file);

        if(chunksize > 0)
        {
            /* send the buffer contents as HTTP response chunk */
            if(httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK)
            {
                /* abort sending file */
                ESP_LOGE(TAGE, "file sending failed!");
                fclose(file);
                free(chunk);
                /* respond with an empty chunk to signal HTTP response completion */
                httpd_resp_send_chunk(req, NULL, 0);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to send file");
                return ESP_FAIL;
            }
        }
        /* keep looping till the whole file is sent */
    }
    while(chunksize != 0);

    /* close file after sending complete */
    fclose(file);
    free(chunk);
    ESP_LOGI(TAG, "file sent done: %s", filename);
    /* respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
