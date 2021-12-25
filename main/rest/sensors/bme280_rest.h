
#include "cJSON.h"
#include <esp_err.h>
#include "esp_http_server.h"
#include "../rest_utils.h"

#include "bme280_driver.h"
cJSON *sensor_data_bme280(httpd_req_t *req);
esp_err_t register_bme280_handlers(httpd_handle_t *server, bme280_driver_t *sensor);