#include "device.h"

cJSON *get_stats_json()
{
    cJSON *stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats, "uptime", esp_timer_get_time() / 1000);        // return uptime in milliseconds
    cJSON_AddNumberToObject(stats, "available_memory", esp_get_free_heap_size()); // return free heap size in bytes
    return stats;
}
esp_err_t stats_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");

    cJSON *root = get_stats_json();

    const char *response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, response);

    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}