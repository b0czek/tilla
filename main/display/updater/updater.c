#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cJSON.h>
#include <esp_log.h>

#include "nvs_utils.h"
#include "updater.h"
#include "layout.h"
#include "remote_sensor.h"

#include "encoder.h"

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

cJSON *get_sync(fetcher_client_t *client, const char *remote_sensor_uuid, uint64_t last_update_timestamp)
{
    char *query = sprintf_query("since=%lld&remote_sensor_uuid=%s", last_update_timestamp, remote_sensor_uuid);
    esp_err_t res = make_http_request(client, SYNC, query);
    free(query);
    ESP_LOGD(TAG, "HTTP FETCH SYNC: %d", res);
    if (res != 0)
    {
        return NULL;
    }
    cJSON *response_json = cJSON_Parse(client->user_data);
    if (!response_json)
    {
        ESP_LOGE(TAG, "error parsing sync json");
        return NULL;
    }
    return response_json;
}

display_updater_t *display_updater_init(SemaphoreHandle_t xGuiSemaphore)
{
    if (!rest_register_check())
    {
        return NULL;
    }

    display_updater_t *updater = calloc(1, sizeof(display_updater_t));
    updater->active_remote_sensor = 0;
    updater->xGuiSemaphore = xGuiSemaphore;
    updater->xUpdaterSemaphore = xSemaphoreCreateMutex();
    updater->client = fetcher_client_init();

    xTaskCreatePinnedToCore(update, "display_updater", 4096, updater, 1, &updater->xHandle, GUI_CORE);

    updater->encoder = encoder_init(updater);
    if (!updater->encoder)
    {
        ESP_LOGE(TAG, "failed to initialize encoder");
    }

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

    ESP_LOGI(TAG, "received display info, %d remote sensor(s) are registered", remote_sensors_count);

    // if there aren't remote sensors, wait infinitely
    if (remote_sensors_count == 0)
    {
        while (true)
        {
            vTaskDelay(UINT32_MAX);
        }
    }

    updater->remote_sensors_count = remote_sensors_count;
    updater->remote_sensors = malloc(sizeof(remote_sensor_data_t) * remote_sensors_count);
    for (int i = 0; i < remote_sensors_count; i++)
    {
        remote_sensor_init_data_static(cJSON_GetArrayItem(remote_sensors_json, i), updater->remote_sensors + i);
    }
    cJSON_Delete(remote_sensors_json);

    // initialize layout
    updater->layout = layout_init(updater->remote_sensors, updater->xGuiSemaphore);
    ESP_LOGI(TAG, "initialized layout");

    while (1)
    {
        // if semaphore could not be locked, try again
        if (xSemaphoreTake(updater->xUpdaterSemaphore, portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "updater semaphore could not be locked");
            continue;
        }
        for (int i = 0; i < updater->remote_sensors_count; i++)
        {
            remote_sensor_data_t *remote_sensor = updater->remote_sensors + i;

            // increment the counter
            remote_sensor->update_counter = (remote_sensor->update_counter + 1) % (remote_sensor->polling_interval / 1000);

            if (remote_sensor->update_counter != 1)
            {
                continue;
            }

            // if there was an error last sync, synchronize whole data
            uint64_t timestamp = remote_sensor->error ? 0 : remote_sensor->last_update_timestamp;
            // fetch data from the server
            cJSON *sync_json = get_sync(updater->client, remote_sensor->remote_sensor_uuid, timestamp);
            // if there was a problem fetching data
            if (!sync_json)
            {
                ESP_LOGE(TAG, "updating sensor %s failed", remote_sensor->remote_sensor_uuid);

                // set an error
                remote_sensor->error = true;
                // and if the error is on active sensor
                if (updater->active_remote_sensor == i)
                {
                    layout_set_error(updater->layout, remote_sensor, lv_palette_main(LV_PALETTE_RED), updater->xGuiSemaphore);
                }
                continue;
            }
            remote_sensors_update_data(sync_json, remote_sensor);
            // if the updated
            if (updater->active_remote_sensor == i)
            {
                ESP_LOGI(TAG, "updating data on screen");
                layout_update_data(updater->layout, remote_sensor, updater->xGuiSemaphore);
            }
            cJSON_Delete(sync_json);
        }

        xSemaphoreGive(updater->xUpdaterSemaphore);

        // increment updater counters every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    layout_free(updater->layout, updater->xGuiSemaphore);
}