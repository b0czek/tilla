#include <cJSON.h>
#include <esp_log.h>
#include "remote_sensor.h"
#include "string.h"
#include <math.h>

#define TAG "remote_sensor"

#ifndef VOID
#define VOID
#endif

#define return_if_error(err, return_value) \
    if (err)                               \
    {                                      \
        return return_value;               \
    }

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

static void remote_sensor_field_clear_values(remote_sensor_field_t *field, int32_t sample_count)
{
    ESP_LOGI(TAG, "clearing values of field with %d samples", sample_count);
    for (int i = 0; i < sample_count; i++)
    {
        *(field->values + i) = INT16_MAX;
    }
}

static void remote_sensor_field_push_value(remote_sensor_field_t *field, int32_t sample_count, int16_t value)
{
    // move whole array left by one element
    memmove(field->values, field->values + 1, (sample_count - 1) * sizeof(int16_t));
    *(field->values + sample_count - 1) = value;
}

int remote_sensor_field_init_data(cJSON *field_json, remote_sensor_field_t *field, int32_t sample_count)
{
    field->label = strdup(cJSON_GetObjectItem(field_json, "label")->valuestring);
    field->name = strdup(cJSON_GetObjectItem(field_json, "name")->valuestring);
    field->unit = strdup(cJSON_GetObjectItem(field_json, "unit")->valuestring);
    if (!field->name || !field->label || !field->unit)
    {
        return -1;
    }
    field->color = (int32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(field_json, "color"));
    field->priority = (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(field_json, "priority"));

    field->range_min = (int16_t)cJSON_GetNumberValue(cJSON_GetObjectItem(field_json, "range_min"));
    field->range_max = (int16_t)cJSON_GetNumberValue(cJSON_GetObjectItem(field_json, "range_max"));

    cJSON *lower_threshold = cJSON_GetObjectItem(field_json, "alarm_lower_threshold");
    field->alarm_lower_threshold = cJSON_IsNull(lower_threshold) ? INT16_MIN : cJSON_GetNumberValue(lower_threshold);
    cJSON *upper_threshold = cJSON_GetObjectItem(field_json, "alarm_upper_threshold");
    field->alarm_upper_threshold = cJSON_IsNull(upper_threshold) ? INT16_MAX : cJSON_GetNumberValue(upper_threshold);

    field->current_value = 0.0;
    field->values = malloc(sample_count * sizeof(int16_t));
    if (!field->values)
    {
        ESP_LOGE(TAG, "COULD NOT ALLOCATE %dB OF MEMORY FOR FIELD VALUES", sample_count * sizeof(int16_t));
        return -1;
    }
    remote_sensor_field_clear_values(field, sample_count);

    return 0;
}

int remote_sensor_init_data_static(cJSON *sensor_json, remote_sensor_data_t *sensor_data)
{
    if (sensor_json == NULL)
    {
        return -1;
    }

    sensor_data->polling_interval = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "polling_interval"));
    sensor_data->max_sample_age = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "max_sample_age"));
    sensor_data->sample_count = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(sensor_json, "sample_count"));

    sensor_data->error = false;
    sensor_data->device_online = false;

    sensor_data->last_update_timestamp = 0;

    sensor_data->update_counter = 0;

    sensor_data->alarm_triggered = false;

    cJSON *fields = cJSON_GetObjectItem(sensor_json, "fields");
    sensor_data->fields_count = cJSON_GetArraySize(fields);
    // calloc for easier deallaction later
    sensor_data->fields = calloc(sensor_data->fields_count, sizeof(remote_sensor_field_t));
    if (!sensor_data->fields)
    {
        return -1;
    }
    for (int i = 0; i < sensor_data->fields_count; i++)
    {
        int init_result = remote_sensor_field_init_data(cJSON_GetArrayItem(fields, i), sensor_data->fields + i, sensor_data->sample_count);
        if (init_result != 0)
        {
            remote_sensor_free_data_static(sensor_data);
            return -1;
        }
    }

    sensor_data->device_name = strdup(cJSON_GetObjectItem(sensor_json, "device_name")->valuestring);
    sensor_data->sensor_name = strdup(cJSON_GetObjectItem(sensor_json, "sensor_name")->valuestring);
    sensor_data->sensor_type = strdup(cJSON_GetObjectItem(sensor_json, "sensor_type")->valuestring);
    sensor_data->remote_sensor_uuid = strdup(cJSON_GetObjectItem(sensor_json, "remote_sensor_uuid")->valuestring);
    if (!sensor_data->device_name || !sensor_data->sensor_name || !sensor_data->sensor_type || !sensor_data->remote_sensor_uuid)
    {
        remote_sensor_free_data_static(sensor_data);
        return -1;
    }

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

    for (int i = 0; i < sensor_data->fields_count; i++)
    {
        remote_sensor_field_t *field = sensor_data->fields + i;
        free(field->label);
        free(field->name);
        free(field->unit);
        free(field->values);
    }
    free(sensor_data->fields);
}

void remote_sensor_free_data(remote_sensor_data_t *sensor_data)
{
    remote_sensor_free_data_static(sensor_data);
    free(sensor_data);
}

void remote_sensor_unload_field_data(cJSON *data_json, remote_sensor_field_t *sensor_field, int32_t new_samples_count, int32_t sample_count)
{
    // base describes how many elements remain unchanged,
    // and determines the offset on setting new values
    int base = sample_count - new_samples_count;
    int move_size = base * sizeof(int16_t);
    // move memory to the beggining of the array,
    // for example if there are 4 new samples in 10 sample array,
    // move 6 latest to the beginning to make room for new ones
    memmove(sensor_field->values, sensor_field->values + new_samples_count, move_size);
    ESP_LOGD(TAG, "moving %d bytes in field data values, new samples: %d", move_size, new_samples_count);

    int data_length = cJSON_GetArraySize(data_json);
    int sample_index = 0;
    for (int i = 0; i < data_length; i++)
    {
        cJSON *el = cJSON_GetArrayItem(data_json, i);
        bool error = cJSON_IsTrue(cJSON_GetObjectItem(el, "error"));
        int count = cJSON_GetNumberValue(cJSON_GetObjectItem(el, "count"));

        for (int k = 0; k < count; k++)
        {
            cJSON *field = cJSON_GetObjectItem(el, sensor_field->name);
            // if value is se to int16_max, point on chart is not displayed
            int16_t value = error || cJSON_IsNull(field) ? INT16_MAX : cJSON_GetNumberValue(field);
            // set values at the end of the array, according to sample_index
            // sample count - new samples count determines
            *(sensor_field->values + base + sample_index) = value;

            sample_index++;
        }
    }
}

void remote_sensor_set_error_state(remote_sensor_data_t *sensor_data, bool error)
{
    sensor_data->error = error;
    // when error is set, push empty value to values array to represent time elapsed since error,
    // upon sucessful sync, whole array will be replaced either way
    if (error)
    {
        for (int i = 0; i < sensor_data->fields_count; i++)
        {
            remote_sensor_field_push_value(sensor_data->fields + i, sensor_data->sample_count, INT16_MAX);
        }
    }
}

void remote_sensor_update_data(cJSON *sync_json, remote_sensor_data_t *sensor_data)
{
    bool was_error = sensor_data->error;

    cJSON *error_json = cJSON_GetObjectItem(sync_json, "error");
    bool error = cJSON_IsInvalid(error_json) || cJSON_IsTrue(error_json);

    remote_sensor_set_error_state(sensor_data, error);
    return_if_error(error, VOID);

    cJSON *sensors = cJSON_GetObjectItem(sync_json, "sensors");

    cJSON *sensor = cJSON_GetObjectItem(sensors, sensor_data->remote_sensor_uuid);

    // set error if there is no sensor with that uuid in response
    remote_sensor_set_error_state(sensor_data, sensor == NULL);
    return_if_error(sensor_data->error, VOID);

    // or if the sensor has an error
    remote_sensor_set_error_state(sensor_data, cJSON_IsTrue(cJSON_GetObjectItem(sensor, "error")));
    return_if_error(sensor_data->error, VOID);

    sensor_data->device_online = cJSON_IsTrue(cJSON_GetObjectItem(sensor, "device_online"));

    sensor_data->last_update_timestamp = cJSON_GetNumberValue(cJSON_GetObjectItem(sensor, "closing_timestamp"));

    cJSON *current_values = cJSON_GetObjectItem(sensor, "current_values");
    cJSON *data = cJSON_GetObjectItem(sensor, "data");

    int32_t new_samples_count = fmin(
        sensor_data->sample_count,
        cJSON_GetNumberValue(cJSON_GetObjectItem(sensor, "new_samples_count")));

    for (int i = 0; i < sensor_data->fields_count; i++)
    {
        remote_sensor_field_t *field = sensor_data->fields + i;
        // update current values, checking for null should not be needed, since at this points,
        // if there was an error, the function would return
        cJSON *current_value = cJSON_GetObjectItem(current_values, field->name);
        field->current_value = cJSON_IsNull(current_value) ? INT16_MAX : cJSON_GetNumberValue(current_value);

        // if there was an error previously, clear whole values array
        if (was_error)
        {
            remote_sensor_field_clear_values(field, sensor_data->sample_count);
        }

        // and update values vec
        remote_sensor_unload_field_data(data, field, new_samples_count, sensor_data->sample_count);
    }
}
