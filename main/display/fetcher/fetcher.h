#pragma once

#include <esp_err.h>
#include <esp_http_client.h>
#include <stdarg.h>

#define USER_DATA_SIZE 10240

typedef enum
{
    INFO = 0,
    DATA,
    SYNC,

} server_endpoints_t;

typedef struct fetcher_client_t
{
    // esp_http_client_config_t http_config;
    esp_http_client_handle_t http_client;
    char *device_uuid;
    char *auth_key;
    char *callback_host;
    char *user_data;
    int callback_port;
} fetcher_client_t;

fetcher_client_t *fetcher_client_init();
void fetcher_client_free(fetcher_client_t *client);
esp_err_t
make_http_request(fetcher_client_t *client, server_endpoints_t endpoint, const char *query_args);

char *sprintf_query(const char *format, ...);