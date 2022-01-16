#include <cJSON.h>
#include "remote_sensor.h"
#include "string.h"

char *strdup(const char *str1)
{
    int str_len = strlen(str1) + 1;
    char *str = malloc(str_len);
    if (str == NULL)
    {
        return NULL;
    }
    memcpy(str, str1, str_len);
    return str;
}

int remote_sensor_init_data_static(cJSON *sensor_json, remote_sensor_data_t *sensor_data)
{
    if (sensor_json == NULL)
    {
        return -1;
    }

    sensor_data->device_name = strdup(cJSON_GetObjectItem(sensor_json, "device_name")->valuestring);
    sensor_data->sensor_name = strdup(cJSON_GetObjectItem(sensor_json, "sensor_name")->valuestring);
    sensor_data->sensor_type = strdup(cJSON_GetObjectItem(sensor_json, "sensor_type")->valuestring);
    sensor_data->remote_sensor_uuid = cJSON_GetObjectItem(sensor_json, "remote_sensor_uuid")->valuestring;
    if (!sensor_data->device_name || !sensor_data->sensor_name || !sensor_data->sensor_type || !sensor_data->remote_sensor_uuid)
    {
        remote_sensor_free_data_static(sensor_data);
        return -1;
    }

    sensor_data->polling_interval = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "polling_interval"));
    sensor_data->max_sample_age = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "max_sample_age"));
    sensor_data->sample_count = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "sample_count"));
    sensor_data->last_update_timestamp = 0;

    return 0;
}

remote_sensor_data_t *remote_sensor_init_data(cJSON *sensor_json)
{
    remote_sensor_data_t *sensor_data = malloc(sizeof(remote_sensor_data_t));

    if (sensor_data == NULL)
    {
        return NULL;
    }

    int result = remote_sensor_init_data_static(sensor_json, sensor_data);

    if (result != 0)
    {
        free(sensor_data);
        return NULL;
    }

    return sensor_data;
}

void remote_sensor_free_data_static(remote_sensor_data_t *sensor_data)
{
    free(sensor_data->device_name);
    free(sensor_data->sensor_name);
    free(sensor_data->sensor_type);
    free(sensor_data->remote_sensor_uuid);
}

void remote_sensor_free_data(remote_sensor_data_t *sensor_data)
{
    remote_sensor_free_data_static(sensor_data);
    free(sensor_data);
}