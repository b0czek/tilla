#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "sensors.h"

cJSON *sensors_data_ds18b20(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    ds18b20_data_t *data = (ds18b20_data_t *)req->user_ctx;
    xSemaphoreTakeOrFail(data->xSemaphore, root);

    cJSON_AddBoolToObject(root, "error", data->error);
    cJSON *sensors = cJSON_CreateObject();
    for (int i = 0; i < data->devices.length; i++)
    {
        ds18b20_device_t *device = (data->devices.data + i);
        const int ID_LENGTH = (16 + 1) * sizeof(char);
        char *id = malloc(ID_LENGTH);
        owb_string_from_rom_code(device->device->rom_code, id, ID_LENGTH);

        cJSON *sensor = cJSON_CreateObject();
        cJSON_AddNumberToObject(sensor, "error", device->error);
        cJSON_AddNumberToObject(sensor, "temperature", device->reading);
        cJSON_AddItemToObject(sensors, id, sensor);
        free(id);
    }
    cJSON_AddItemToObject(root, "sensors", sensors);
    xSemaphoreGive(data->xSemaphore);
    return root;
}

json_handler_auth(handle_ds18b20_data, sensors_data_ds18b20);

cJSON *config_data_ds18b20(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    ds18b20_data_t *data = (ds18b20_data_t *)req->user_ctx;

    xSemaphoreTakeOrFail(data->xSemaphore, root);

    cJSON_AddNumberToObject(root, "reading_interval", data->config->reading_interval);
    cJSON_AddNumberToObject(root, "resolution", data->config->resolution);
    cJSON_AddNumberToObject(root, "gpio", data->config->gpio);

    xSemaphoreGive(data->xSemaphore);

    return root;
}

json_handler_auth(handle_ds18b20_config, config_data_ds18b20);

esp_err_t register_ds18b20_handlers(httpd_handle_t server, ds18b20_data_t *ds_data)
{
    esp_err_t result = 0;
    httpd_uri_t sensors_data_uri = {
        .uri = "/api/v1/sensors/ds18b20/?",
        .method = HTTP_GET,
        .handler = handle_ds18b20_data,
        .user_ctx = ds_data};
    result += httpd_register_uri_handler(server, &sensors_data_uri);

    httpd_uri_t sensors_config_uri = {
        .uri = "/api/v1/sensors/ds18b20/config/?",
        .method = HTTP_GET,
        .handler = handle_ds18b20_config,
        .user_ctx = ds_data};
    result += httpd_register_uri_handler(server, &sensors_config_uri);
    return result;
}
