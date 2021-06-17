#include <esp_http_server.h>

#include "device.h"

/* Handler for getting all info about device */
esp_err_t device_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "stats", get_stats_json());
    cJSON_AddItemToObject(root, "interfaces_info", get_network_info_json());
    cJSON_AddItemToObject(root, "chip_info", get_chip_info_json());

    const char *response = cJSON_Print(root);
    httpd_resp_sendstr(req, response);

    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t register_device_handlers(httpd_handle_t server, rest_server_context_t *ctx)
{
    httpd_uri_t device_data_get_uri = {
        .uri = "/api/v1/device/?",
        .method = HTTP_GET,
        .handler = device_data_get_handler,
        .user_ctx = ctx};
    httpd_register_uri_handler(server, &device_data_get_uri);

    httpd_uri_t chip_data_get_uri = {
        .uri = "/api/v1/device/chip/?",
        .method = HTTP_GET,
        .handler = chip_data_get_handler,
        .user_ctx = ctx};
    httpd_register_uri_handler(server, &chip_data_get_uri);

    httpd_uri_t network_data_get_uri = {
        .uri = "/api/v1/device/network/?",
        .method = HTTP_GET,
        .handler = network_data_get_handler,
        .user_ctx = ctx};
    httpd_register_uri_handler(server, &network_data_get_uri);

    httpd_uri_t stats_data_get_uri = {
        .uri = "/api/v1/device/stats/?",
        .method = HTTP_GET,
        .handler = stats_data_get_handler,
        .user_ctx = ctx};
    httpd_register_uri_handler(server, &stats_data_get_uri);
    return ESP_OK;
}