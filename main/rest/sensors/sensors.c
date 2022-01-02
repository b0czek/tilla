#include "sensors.h"
#include <cJSON.h>
#include "drivers.h"
#include <esp_http_server.h>

cJSON *sensors_data(httpd_req_t *req)
{
    sensor_drivers_t *driver = (sensor_drivers_t *)req->user_ctx;
    cJSON *root = cJSON_CreateObject();

#ifdef CONFIG_DS18B20_ENABLED

    req->user_ctx = driver->ds18b20_driver;
    cJSON_AddItemToObject(root, "ds18b20", sensors_data_ds18b20(req));
#endif

#ifdef CONFIG_BME280_ENABLED
    req->user_ctx = driver->bme280_driver;
    cJSON_AddItemToObject(root, "bme280", sensors_data_bme280(req));
#endif
    req->user_ctx = driver;
    return root;
}
json_handler_auth(sensors_data_handler, sensors_data);

esp_err_t register_sensor_handlers(httpd_handle_t server, sensor_drivers_t *sensors)
{
    esp_err_t result = 0;
#ifdef CONFIG_DS18B20_ENABLED
    result += register_ds18b20_handlers(server, sensors->ds18b20_driver);
#endif

#ifdef CONFIG_BME280_ENABLED
    result += register_bme280_handlers(server, sensors->bme280_driver);
#endif

    httpd_uri_t sensor_data_uri = {
        .uri = "/api/v1/sensors/?",
        .method = HTTP_GET,
        .handler = sensors_data_handler,
        .user_ctx = sensors};
    result += httpd_register_uri_handler(server, &sensor_data_uri);

    return result;
}