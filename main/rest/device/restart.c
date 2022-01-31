#include <esp_http_server.h>
#include <cJSON.h>
#include "rest_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
cJSON *device_restart(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);

    int sockfd = httpd_req_to_sockfd(req);

    // make sure the response was sent properly and close the socket
    if (respond_json(req, root) == ESP_OK && httpd_sess_trigger_close(req->handle, sockfd) == ESP_OK)
    {
        // wait a second before restarting
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }

    return NULL;
}