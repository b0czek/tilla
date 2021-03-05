/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "esp_network.h"

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
static esp_err_t temperature_data_get_handler(httpd_req_t *req)
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

/* Handler for getting info about device */
static esp_err_t device_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000); // return uptime in milliseconds

    int network_interfaces_count = 0;
#ifdef CONFIG_CONNECT_ETHERNET
    network_interfaces_count++;
#endif
#ifdef CONFIG_CONNECT_WIFI
    network_interfaces_count++;
#endif
    esp_network_info_t *interfaces_info = malloc(network_interfaces_count * sizeof(esp_network_info_t));
    get_esp_network_info(interfaces_info, network_interfaces_count);
    cJSON *interfaces = cJSON_CreateObject();

    for (int i = 0; i < network_interfaces_count; i++)
    {
        esp_network_info_t *netif = interfaces_info + i;
        cJSON *interface = cJSON_CreateObject();
        cJSON_AddStringToObject(interface, "mac_address", netif->mac);

        cJSON_AddStringToObject(interface, "ip", netif->ip);
        cJSON_AddStringToObject(interface, "netmask", netif->netmask);
        cJSON_AddStringToObject(interface, "gw", netif->gw);
        cJSON_AddStringToObject(interface, "primary_dns", netif->dns_primary);
        cJSON_AddStringToObject(interface, "secondary_dns", netif->dns_secondary);

        cJSON_AddBoolToObject(interface, "is_up", netif->is_up);
        if (strcmp(netif->desc, "sta") == 0)
        {
            cJSON *wifi_info = cJSON_CreateObject();
            cJSON_AddStringToObject(wifi_info, "ssid", (char *)netif->wifi_info->ssid);
            cJSON_AddNumberToObject(wifi_info, "rssi", netif->wifi_info->rssi);
            cJSON_AddNumberToObject(wifi_info, "auth_mode", (int)netif->wifi_info->authmode);
            cJSON_AddStringToObject(wifi_info, "country_code", netif->wifi_info->country.cc);
            cJSON_AddItemToObject(interface, "wifi_info", wifi_info);
        }
        cJSON_AddItemToObject(interfaces, netif->desc, interface);
    }
    cJSON_AddItemToObject(root, "interfaces_info", interfaces);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON *chip = cJSON_CreateObject();
    cJSON_AddNumberToObject(chip, "chip_model", (int)chip_info.model);
    cJSON_AddNumberToObject(chip, "features", chip_info.features);
    cJSON_AddNumberToObject(chip, "cores", chip_info.cores);
    cJSON_AddNumberToObject(chip, "revision", chip_info.revision);

    cJSON_AddItemToObject(root, "chip", chip);

    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);

    free((void *)sys_info);
    cJSON_Delete(root);
    // for (int i = 0; i < network_interfaces_count; i++)
    // {
    //     free_esp_network_info(interfaces_info);
    // }
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

    /* URI handler for fetching uptime data */
    httpd_uri_t device_data_get_uri = {
        .uri = "/api/v1/device",
        .method = HTTP_GET,
        .handler = device_data_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &device_data_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
