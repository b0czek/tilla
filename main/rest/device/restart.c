#include <esp_http_server.h>
#include <cJSON.h>
#include "rest_utils.h"

cJSON *device_restart(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);

    // make sure the response was sent properly
    if (respond_json(req, root) == ESP_OK)
    {
        esp_restart();
    }

    return NULL;
}