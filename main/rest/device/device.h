#pragma once

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include "rest_server.h"
#include "network.h"
#include "../rest_utils.h"

// chip functions
cJSON *get_chip_info_json(httpd_req_t *req);

// network
#if defined(CONFIG_CONNECT_ETHERNET) && defined(CONFIG_CONNECT_WIFI)
#define NETWORK_INTERFACES_COUNT 2
#elif defined(CONFIG_CONNECT_ETHERNET) || defined(CONFIG_CONNECT_WIFI)
#define NETWORK_INTERFACES_COUNT 1
#else
// ?? whats the point
#define NETWORK_INTERFACES_COUNT 0
#endif

esp_network_info_t *get_network_info();
cJSON *get_network_info_json(httpd_req_t *req);

// stats
cJSON *get_stats_json(httpd_req_t *req);

// device functions
esp_err_t register_device_handlers(httpd_handle_t *server);