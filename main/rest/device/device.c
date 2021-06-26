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

esp_err_t register_device_handlers(json_node_t *server)
{
    json_node_endpoint_t device_data_get_uri = {

        .handler = device_data_get_json,
        .method = HTTP_GET,
    };
    json_node_t *device = json_endpoint_add(server, "device", &device_data_get_uri);
    printf("device endpoint: %s\n", device->uri_fragment);
    json_node_endpoint_t chip_data_get_uri = {

        .handler = get_chip_info_json,
        .method = HTTP_GET,
    };
    json_endpoint_add(device, "chip", &chip_data_get_uri);

    json_node_endpoint_t network_data_get_uri = {

        .handler = get_network_info_json,
        .method = HTTP_GET,
    };
    json_endpoint_add(device, "network", &network_data_get_uri);

    json_node_endpoint_t stats_data_get_uri = {

        .handler = get_stats_json,
        .method = HTTP_GET,
    };
    json_endpoint_add(device, "stats", &stats_data_get_uri);

    return ESP_OK;
}