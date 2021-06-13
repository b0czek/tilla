#include "device.h"

cJSON *get_stats_json()
{
    cJSON *stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats, "uptime", esp_timer_get_time() / 1000);        // return uptime in milliseconds
    cJSON_AddNumberToObject(stats, "available_memory", esp_get_free_heap_size()); // return free heap size in bytes
    return stats;
}
