#pragma once

#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>

bool rest_register_check();
bool rest_auth_check(httpd_req_t *req);
cJSON *unauthenticated_handler(httpd_req_t *req);
esp_err_t common_handler(httpd_req_t *req, cJSON *(fn)(httpd_req_t *));
esp_err_t respond_json(httpd_req_t *req, cJSON *res);

#define json_handler(fn_name, fn)              \
    static esp_err_t fn_name(httpd_req_t *req) \
    {                                          \
        return common_handler(req, fn);        \
    }
#define json_handler_auth(fn_name, fn)                                                   \
    static esp_err_t fn_name(httpd_req_t *req)                                           \
    {                                                                                    \
        return common_handler(req, rest_auth_check(req) ? fn : unauthenticated_handler); \
    }

#define json_handler_auth_if_registered(fn_name, fn)                                                                  \
    static esp_err_t fn_name(httpd_req_t *req)                                                                        \
    {                                                                                                                 \
        return common_handler(req, rest_register_check() ? rest_auth_check(req) ? fn : unauthenticated_handler : fn); \
    }
