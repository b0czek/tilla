#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#include "rest_server.h"
#include "device.h"
#include "ds18b20_driver.h"
#include "ds18b20_rest.h"
#include "bme280_driver.h"
#include "bme280_rest.h"
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

    // /* URI handlers for fetching data about device vitals */
    register_device_handlers(&server);

    /* URI handler for fetching device restart */
    httpd_uri_t device_restart_uri = {
        .uri = "/api/v1/restart",
        .method = HTTP_GET,
        .handler = device_restart_handler};
    httpd_register_uri_handler(server, &device_restart_uri);

#ifdef CONFIG_DS18B20_ENABLED
    register_ds18b20_handlers(&server, sensors->ds18b20_driver);
#endif

#ifdef CONFIG_BME280_ENABLED
    register_bme280_handlers(&server, sensors->bme280_driver);
#endif

    register_registration_handlers(&server);

    return ESP_OK;
err_start:
    // free(rest_context);
    return ESP_FAIL;
}
