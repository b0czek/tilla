
#include "sensors.h"
#include <stdlib.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

void format_addr(char *buf, bme280_dev_config_t *config)
{
    sprintf(buf, "0x%.2x@%d", config->addr, config->i2c_port);
}

cJSON *sensors_data_bme280(httpd_req_t *req)
{
    bme280_driver_t *driver = (bme280_driver_t *)req->user_ctx;

    cJSON *root = cJSON_CreateObject();

    xSemaphoreTakeOrFail(driver->xSemaphore, root);

    cJSON_AddNumberToObject(root, "error", driver->error);
    cJSON *sensors_data = cJSON_CreateObject();
    for (int i = 0; i < driver->sensors.length; i++)
    {
        cJSON *sensor_data = cJSON_CreateObject();
        bme280_sensor_t *sensor = driver->sensors.data + i;
        char addr[20] = {0};
        format_addr(addr, &sensor->config);

        cJSON_AddNumberToObject(sensor_data, "error", sensor->error);
        cJSON_AddNumberToObject(sensor_data, "temperature", sensor->data.temperature);
        cJSON_AddNumberToObject(sensor_data, "humidity", sensor->data.humidity);
        cJSON_AddNumberToObject(sensor_data, "pressure", sensor->data.pressure);

        cJSON_AddItemToObject(sensors_data, addr, sensor_data);
    }

    cJSON_AddItemToObject(root, "sensors", sensors_data);

    xSemaphoreGive(driver->xSemaphore);
    return root;
}

json_handler_auth(handle_bme280_data, sensors_data_bme280);

esp_err_t register_bme280_handlers(httpd_handle_t server, bme280_driver_t *driver)
{
    esp_err_t result = 0;
    httpd_uri_t sensor_data_uri = {
        .uri = "/api/v1/sensors/bme280/?",
        .method = HTTP_GET,
        .handler = handle_bme280_data,
        .user_ctx = driver};
    result += httpd_register_uri_handler(server, &sensor_data_uri);
    return result;
}