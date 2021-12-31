#pragma once
#include "drivers.h"
// #include <esp_vfs.h>

// #define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
// #define SCRATCH_BUFSIZE (10240)

// typedef struct rest_server_context
// {
//     char base_path[ESP_VFS_PATH_MAX + 1];
//     char scratch[SCRATCH_BUFSIZE];
// } rest_server_context_t;

esp_err_t start_rest_server(sensor_drivers_t *sensors);