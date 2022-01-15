#pragma once
#include <esp_err.h>
#include "stdlib.h"
#include <stdbool.h>

char *read_nvs_str(const char *key);
esp_err_t read_nvs_int(const char *key, int32_t *out_value);

bool rest_register_check();