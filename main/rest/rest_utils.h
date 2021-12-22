#pragma once

#include <esp_http_server.h>
#include <cJSON.h>

#define json_handler(fn_name, fn)                                       \
    static esp_err_t fn_name(httpd_req_t *req)                          \
    {                                                                   \
        httpd_resp_set_type(req, "application/json");                   \
        cJSON *response = fn(req);                                      \
        const char *response_string = cJSON_PrintUnformatted(response); \
        httpd_resp_sendstr(req, response_string);                       \
        free((void *)response_string);                                  \
        cJSON_Delete(response);                                         \
        return ESP_OK;                                                  \
    }
