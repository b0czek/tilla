#pragma once

#include "cJSON.h"
#include <esp_err.h>
#include "esp_http_server.h"
#include "../rest_utils.h"

#include "ds18b20_driver.h"

cJSON *sensors_data_ds18b20(httpd_req_t *req);
esp_err_t register_ds18b20_handlers(httpd_handle_t *server, ds18b20_data_t *ds_data);