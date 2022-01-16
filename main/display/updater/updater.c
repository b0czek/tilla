#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cJSON.h>
#include <esp_log.h>

#include "nvs_utils.h"
#include "updater.h"
#include "remote_sensor.h"

#define TAG "updater"
static void update(void *arg);

cJSON *get_init_info(fetcher_client_t *client)
{
    esp_err_t res = make_http_request(client, INFO, NULL);

    if (res != ESP_OK)
    {
        return NULL;
    }

    cJSON *info_root = cJSON_Parse(client->user_data);
    if (!info_root || cJSON_IsInvalid(info_root))
    {
        goto json_err;
    }
    cJSON *error = cJSON_GetObjectItem(info_root, "error");
    if (!cJSON_IsBool(error) || cJSON_IsTrue(error))
    {
        goto json_err;
    }

    return info_root;
json_err:
    cJSON_Delete(info_root);
    return NULL;
}

display_updater_t *display_updater_init(SemaphoreHandle_t xGuiSemaphore)
{
    if (!rest_register_check())
    {
        return NULL;
    }

    display_updater_t *updater = calloc(1, sizeof(display_updater_t));
    updater->xGuiSemaphore = xGuiSemaphore;
    updater->client = fetcher_client_init();

    xTaskCreatePinnedToCore(update, "display_updater", 4096, updater, 0, &updater->xHandle, 1);

    return updater;
}

static void update(void *arg)
{
    display_updater_t *updater = arg;

    cJSON *info = NULL;
    do
    {
        info = get_init_info(updater->client);
        if (info == NULL)
        {
            vTaskDelay(UPDATER_TIME_BETWEEN_INIT_ATTEMPTS / portTICK_PERIOD_MS);
        }
    } while (info == NULL);

    cJSON *remote_sensors_json = cJSON_GetObjectItem(info, "sensors");
    int remote_sensors_count = cJSON_GetArraySize(remote_sensors_json);
    updater->remote_sensors_count = remote_sensors_count;

    ESP_LOGI(TAG, "received display info, %d remote sensor(s) are registered", remote_sensors_count);

    updater->remote_sensors = malloc(sizeof(remote_sensor_data_t) * remote_sensors_count);
    for (int i = 0; i < remote_sensors_count; i++)
    {
        remote_sensor_init_data_static(cJSON_GetArrayItem(remote_sensors_json, i), updater->remote_sensors + i);
    }

    while (1)
    {

        // if (rest_register_check())
        // {
        char *query = sprintf_query("since=%d", updater->remote_sensors->last_update_timestamp);
        esp_err_t res = make_http_request(updater->client, SYNC, query);
        free(query);
        ESP_LOGI(TAG, "HTTP FETCH: %d", res);
        if (res == 0)
        {
            ESP_LOGI(TAG, "sync result: %s", updater->client->user_data);
        }
        // }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}