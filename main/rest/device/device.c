#include "esp_http_server.h"

#include "device.h"

/* Handler for getting info about device */
esp_err_t device_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "stats", get_stats_json());
    cJSON_AddItemToObject(root, "interfaces_info", get_network_info_json());
    cJSON_AddItemToObject(root, "chip_info", get_chip_info_json());

    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);

    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}