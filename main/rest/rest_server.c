#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "rest_server.h"
#include "device.h"

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

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)
typedef struct rest_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

float get_readings();
bool get_error_state();

/* Simple handler for getting temperature data */
static esp_err_t
temperature_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "error", get_error_state());
    cJSON_AddNumberToObject(root, "reading", get_readings());
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t device_restart_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);

    ESP_LOGW("device_restart_handler", "requested device restart");
    esp_restart();
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching temperature data */
    httpd_uri_t temperature_data_get_uri = {
        .uri = "/api/v1/temp",
        .method = HTTP_GET,
        .handler = temperature_data_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &temperature_data_get_uri);

    /* URI handler for fetching data about device vitals */
    httpd_uri_t device_data_get_uri = {
        .uri = "/api/v1/device",
        .method = HTTP_GET,
        .handler = device_data_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &device_data_get_uri);

    httpd_uri_t reset_device_uri = {
        .uri = "/api/v1/reset",
        .method = HTTP_GET,
        .handler = device_restart_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &reset_device_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}