#pragma once

#include "cJSON.h"
#include <esp_err.h>
#include "esp_http_server.h"
#include "../rest_utils.h"

#define MAX_POST_CONTENT_LENGTH 4096
#define AUTH_KEY_LENGTH 32

esp_err_t register_registration_handlers(httpd_handle_t *server);