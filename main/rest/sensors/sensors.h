#pragma once

#include <cJSON.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../rest_utils.h"
#include "drivers.h"

#define xSemaphoreTakeOrFail(semaphore, cjson)              \
    if (xSemaphoreTake(semaphore, portMAX_DELAY) != pdTRUE) \
    {                                                       \
        cJSON_AddBoolToObject(cjson, "error", true);        \
        return cjson;                                       \
    }

#ifdef CONFIG_DS18B20_ENABLED
#include "ds18b20_driver.h"
cJSON *sensors_data_ds18b20(httpd_req_t *req);
esp_err_t register_ds18b20_handlers(httpd_handle_t server, ds18b20_data_t *ds_data);
#endif

#ifdef CONFIG_BME280_ENABLED
#include "bme280_driver.h"
cJSON *sensors_data_bme280(httpd_req_t *req);
esp_err_t register_bme280_handlers(httpd_handle_t server, bme280_driver_t *sensor);
#endif

esp_err_t register_sensor_handlers(httpd_handle_t server, sensor_drivers_t *sensors);