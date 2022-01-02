#include "device.h"

cJSON *get_stats_json(httpd_req_t *req)
{
    cJSON *stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats, "uptime", esp_timer_get_time() / 1000);        // return uptime in milliseconds
    cJSON_AddNumberToObject(stats, "available_memory", esp_get_free_heap_size()); // return free heap size in bytes
    cJSON_AddStringToObject(stats, "idf_version", esp_get_idf_version());         // return idf versio
    return stats;
}
