#include <esp_http_server.h>

#include "device.h"

/* Handler for getting all info about device */
cJSON *device_data_get_json(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "stats", get_stats_json(req));
    cJSON_AddItemToObject(root, "interfaces_info", get_network_info_json(req));
    cJSON_AddItemToObject(root, "chip_info", get_chip_info_json(req));
    return root;
}

json_handler_auth(handle_device_info, device_data_get_json);
json_handler_auth(handle_chip_info, get_chip_info_json);
json_handler_auth(handle_network_info, get_network_info_json);
json_handler_auth(handle_stats_info, get_stats_json);

esp_err_t register_device_handlers(httpd_handle_t *server)
{

    httpd_uri_t device_info_uri = {
        .uri = "/api/v1/device/?",
        .method = HTTP_GET,
        .handler = handle_device_info};
    httpd_register_uri_handler(*server, &device_info_uri);

    httpd_uri_t chip_info_uri = {
        .uri = "/api/v1/device/chip/?",
        .method = HTTP_GET,
        .handler = handle_chip_info};
    httpd_register_uri_handler(*server, &chip_info_uri);

    httpd_uri_t network_info_uri = {
        .uri = "/api/v1/device/network/?",
        .method = HTTP_GET,
        .handler = handle_network_info};
    httpd_register_uri_handler(*server, &network_info_uri);

    httpd_uri_t stats_info_uri = {
        .uri = "/api/v1/device/stats/?",
        .method = HTTP_GET,
        .handler = handle_stats_info};
    httpd_register_uri_handler(*server, &stats_info_uri);

    return ESP_OK;
}