
#include "nvs_flash.h"
#include "stdlib.h"
#include "esp_log.h"

#define NVS_TAG "nvs_utils"

char *read_nvs_str(const char *key)
{
    size_t length;
    nvs_handle_t handle;
    nvs_open("rest", NVS_READONLY, &handle);

    if (nvs_get_str(handle, key, 0, &length) != ESP_OK)
    {
        ESP_LOGW(NVS_TAG, "could not read key length");
        return NULL;
    }
    char *str = malloc(length);

    esp_err_t nvs_read = nvs_get_str(handle, key, str, &length);
    if (nvs_read != ESP_OK)
    {
        ESP_LOGW(NVS_TAG, "could not read key %s from nvs - %d", key, nvs_read);
        free(str);
        return NULL;
    }

    nvs_close(handle);
    return str;
}

esp_err_t read_nvs_int(const char *key, int32_t *out_value)
{
    nvs_handle_t handle;
    nvs_open("rest", NVS_READONLY, &handle);
    esp_err_t result = nvs_get_i32(handle, key, out_value);
    if (result != ESP_OK)
    {
        ESP_LOGW(NVS_TAG, "could not read key %s from nvs - %d", key, result);
    }
    nvs_close(handle);
    return ESP_OK;
}

bool rest_register_check()
{
    size_t length;
    nvs_handle_t handle;
    nvs_open("rest", NVS_READONLY, &handle);
    bool res = nvs_get_str(handle, "auth_key", 0, &length) == ESP_OK;
    nvs_close(handle);
    return res;
}