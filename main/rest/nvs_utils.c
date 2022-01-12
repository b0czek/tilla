
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
