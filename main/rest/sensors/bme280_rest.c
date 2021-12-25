
#include "bme280_rest.h"
#include <stdlib.h>
#include <stdio.h>

cJSON *sensor_data_bme280(httpd_req_t *req)
{
    bme280_driver_t *driver = (bme280_driver_t *)req->user_ctx;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "error", driver->error);
    cJSON *sensors_data = cJSON_CreateObject();
    for (int i = 0; i < driver->sensors.length; i++)
    {
        cJSON *sensor_data = cJSON_CreateObject();
        bme280_sensor_t *sensor = driver->sensors.data + i;
        char addr[20] = {0};
        sprintf(addr, "0x%.2x@%d", sensor->config.addr, sensor->config.i2c_port);

        cJSON_AddNumberToObject(sensor_data, "error", sensor->error);
        cJSON_AddNumberToObject(sensor_data, "temperature", sensor->data.temperature);
        cJSON_AddNumberToObject(sensor_data, "humidity", sensor->data.humidity);
        cJSON_AddNumberToObject(sensor_data, "pressure", sensor->data.pressure);

        cJSON_AddItemToObject(sensors_data, addr, sensor_data);
    }

    cJSON_AddItemToObject(root, "sensors", sensors_data);
    return root;
}

json_handler(handle_bme280_data, sensor_data_bme280);

esp_err_t register_bme280_handlers(httpd_handle_t *server, bme280_driver_t *driver)
{
    httpd_uri_t sensor_data_uri = {
        .uri = "/api/v1/sensors/bme280/?",
        .method = HTTP_GET,
        .handler = handle_bme280_data,
        .user_ctx = driver};
    httpd_register_uri_handler(*server, &sensor_data_uri);
    return ESP_OK;
}