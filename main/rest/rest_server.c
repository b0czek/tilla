#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#include "rest_server.h"
#include "device.h"
#include "sensors.h"
#include "registration.h"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

cJSON *api_version(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "api_version", API_VERSION);
    return root;
}
json_handler(handle_api_version, api_version);

esp_err_t start_rest_server(sensor_drivers_t *sensors)
{
    // REST_CHECK(base_path, "wrong base path", err);
    // rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    // REST_CHECK(rest_context, "No memory for rest context", err);
    // strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.uri_match_fn = httpd_uri_match_wildcard;
    // config.global_user_ctx = rest_context;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    httpd_uri_t api_info_uri = {
        .uri = "/api/?",
        .method = HTTP_GET,
        .handler = handle_api_version};
    httpd_register_uri_handler(server, &api_info_uri);

    /* URI handlers for fetching data about device vitals */
    register_device_handlers(server);

    register_sensor_handlers(server, sensors);

    register_registration_handlers(server);

    return ESP_OK;
err_start:
    // free(rest_context);
    return ESP_FAIL;
}
