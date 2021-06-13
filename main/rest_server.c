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

#include "esp_wifi.h"
#include "network_tools.h"
#include "network_info.h"

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

cJSON *get_chip_info()
{
    cJSON *chip = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddNumberToObject(chip, "chip_model", (int)chip_info.model);
    cJSON_AddNumberToObject(chip, "features", chip_info.features);
    cJSON_AddNumberToObject(chip, "cores", chip_info.cores);
    cJSON_AddNumberToObject(chip, "revision", chip_info.revision);

    // add chip_id(factory programmed mac address)
    uint8_t mac[MAC_BYTES];
    esp_efuse_mac_get_default(mac);
    int id_len = MAC_STRING_LENGH;
    char id[id_len];
    create_mac_string(id, id_len, mac, MAC_BYTES);
    cJSON_AddStringToObject(chip, "chip_id", id);

    return chip;
}
cJSON *chip_info = NULL;

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
    esp_err_t network_info_result = get_esp_network_info(interfaces_info, network_interfaces_count);
    cJSON *interfaces = cJSON_CreateObject();

    for (int i = 0; i < network_interfaces_count; i++)
    {
        esp_network_info_t *netif = interfaces_info + i;
        cJSON *interface = cJSON_CreateObject();
        cJSON_AddStringToObject(interface, "mac_address", netif->mac);

        cJSON *ip_info = cJSON_CreateObject();
        cJSON_AddStringToObject(ip_info, "ip", netif->ip_info.ip);
        cJSON_AddStringToObject(ip_info, "netmask", netif->ip_info.netmask);
        cJSON_AddStringToObject(ip_info, "gw", netif->ip_info.gw);
        cJSON_AddItemToObject(interface, "ip_info", ip_info);

        cJSON *dns_info = cJSON_CreateObject();
        cJSON_AddStringToObject(dns_info, "primary_dns", netif->dns_info.main);
        cJSON_AddStringToObject(dns_info, "secondary_dns", netif->dns_info.backup);
        cJSON_AddItemToObject(interface, "dns", dns_info);
        cJSON_AddStringToObject(interface, "hostname", netif->hostname);

        cJSON_AddBoolToObject(interface, "connected", netif->connected);
        if (strcmp(netif->desc, "sta") == 0) // if netif is wifi client
        {
            cJSON *wifi_info = cJSON_CreateObject(); // create json object
            if (netif->wifi_info)                    // if pointer is not null, then add info about connection
            {
                int id_len = MAC_STRING_LENGH;
                char id[id_len];
                create_mac_string(id, id_len, netif->wifi_info->bssid, MAC_BYTES);
                cJSON_AddStringToObject(wifi_info, "bssid", id);

                cJSON_AddStringToObject(wifi_info, "ssid", (char *)netif->wifi_info->ssid);
                cJSON_AddNumberToObject(wifi_info, "rssi", netif->wifi_info->rssi);
                cJSON_AddNumberToObject(wifi_info, "auth_mode", (int)netif->wifi_info->authmode);
                cJSON_AddStringToObject(wifi_info, "country_code", netif->wifi_info->country.cc);
            }
            cJSON_AddItemToObject(interface, "wifi_info", wifi_info);
        }
        cJSON_AddItemToObject(interfaces, netif->desc, interface);
    }
    cJSON_AddItemToObject(root, "interfaces_info", interfaces);

    cJSON_AddItemReferenceToObject(root, "chip_info", chip_info);

    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);

    free((void *)sys_info);
    cJSON_Delete(root);
    free_esp_network_info(interfaces_info, network_interfaces_count);
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

    // prefetch chip info data as it's constant
    chip_info = get_chip_info();

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
