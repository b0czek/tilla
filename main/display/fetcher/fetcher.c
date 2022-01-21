#include "nvs_utils.h"
#include "fetcher.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#define TAG "http_fetcher"

static const char *const endpoint_uris[] = {
    [INFO] = "/api/node/display/info",
    [DATA] = "/api/node/display/data",
    [SYNC] = "/api/node/display/sync",
};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len; // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (output_len + evt->data_len > USER_DATA_SIZE)
            {
                return -1;
            }
            memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
            // ESP_LOGW(TAG, "free ram: %d, output len: %d, largest heap block: %d", esp_get_free_heap_size(), output_len, heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        // add null at the end of data string
        *((char *)evt->user_data + output_len + evt->data_len) = '\0';
        ESP_LOGD(TAG, "response: %s", (char *)evt->user_data);

        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        output_len = 0;
        break;
    }
    return ESP_OK;
}

char *sprintf_query(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    // get needed string size
    int str_size = vsnprintf(NULL, 0, format, args);
    if (str_size < 0)
    {
        return NULL;
    }
    // account for \0
    str_size++;
    char *result = malloc(str_size);
    int r = vsnprintf(result, str_size, format, args);

    if (r < 0)
    {
        free(result);
        return NULL;
    }

    va_end(args);
    return result;
}

char *build_url(fetcher_client_t *client, const char *path, const char *query_args)
{
    if (query_args == NULL)
    {
        query_args = "";
    }
    char *url = sprintf_query("http://%s:%d%s?device_uuid=%s&auth_key=%s&%s",
                              client->callback_host,
                              client->callback_port,
                              path,
                              client->device_uuid,
                              client->auth_key,
                              query_args);

    return url;
}

esp_err_t
make_http_request(fetcher_client_t *client, server_endpoints_t endpoint, const char *query_args)
{
    char *url = build_url(client, endpoint_uris[endpoint], query_args);

    if (url == NULL)
    {
        ESP_LOGE(TAG, "url for endpoint %s could not be built", endpoint_uris[endpoint]);
        return -1;
    }
    esp_http_client_set_url(client->http_client, url);
    free(url);

    ESP_LOGD(TAG, "querying: http://%s:%d%s",
             client->callback_host,
             client->callback_port,
             endpoint_uris[endpoint]);

    esp_err_t result = esp_http_client_perform(client->http_client);

    // esp handles http requests much better when they are closed each time
    // when they are not, there are many problems with tcp stuff
    esp_http_client_close(client->http_client);

    int content_length = esp_http_client_get_content_length(client->http_client);
    if (content_length > USER_DATA_SIZE)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return result;
}

fetcher_client_t *fetcher_client_init()
{

    int callback_port;
    esp_err_t callback_port_read_result = read_nvs_int("callback_port", &callback_port);

    char *callback_host = read_nvs_str("callback_host");
    char *device_uuid = read_nvs_str("device_uuid");
    char *auth_key = read_nvs_str("auth_key");

    if (callback_host == NULL || device_uuid == NULL || auth_key == NULL || callback_port_read_result != ESP_OK)
    {
        free(callback_host);
        free(device_uuid);
        free(auth_key);
        return NULL;
    }

    fetcher_client_t *client = calloc(1, sizeof(fetcher_client_t));
    if (client == NULL)
    {
        ESP_LOGE(TAG, "insufficient memory to allocate fetcher client");
        return NULL;
    }
    // read_nvs_str result is malloced so their pointer can just be assgined
    client->device_uuid = device_uuid;
    client->auth_key = auth_key;
    client->callback_host = callback_host;
    client->callback_port = callback_port;

    client->user_data = malloc(USER_DATA_SIZE);

    esp_http_client_config_t config = {
        .host = callback_host,
        .port = callback_port,
        .path = endpoint_uris[INFO],
        .event_handler = _http_event_handler,
        .user_data = client->user_data,
        .buffer_size = 4096,
    };
    // same goes for the http client
    client->http_client = esp_http_client_init(&config);
    return client;
}

void fetcher_client_free(fetcher_client_t *client)
{
    free(client->device_uuid);
    free(client->auth_key);
    free(client->callback_host);
    free(client->user_data);

    esp_http_client_cleanup(client->http_client);
}