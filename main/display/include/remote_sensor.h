#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <cJSON.h>

typedef struct remote_sensor_field_t
{
    char *name;
    char *label;
    char *unit;
    uint32_t color;
    uint8_t priority;
    int16_t range_min;
    int16_t range_max;

    double current_value;
    int16_t *values;

} remote_sensor_field_t;

typedef struct remote_sensor_data_t
{
    char *device_name;
    char *sensor_name;
    char *sensor_type;
    char *remote_sensor_uuid;
    int32_t polling_interval;
    int32_t max_sample_age;
    int32_t sample_count;

    remote_sensor_field_t *fields;
    int8_t fields_count;

    bool error;
    bool device_online;

    int32_t update_counter;

    uint64_t last_update_timestamp;

} remote_sensor_data_t;

// initializes data in passed struct, although there are still some mallocs needed for struct members
int remote_sensor_init_data_static(cJSON *sensor_json, remote_sensor_data_t *sensor_data);

// initializes data and returns malloced struct
remote_sensor_data_t *remote_sensor_init_data(cJSON *sensor_json);

// frees struct members, but not the struct itself
void remote_sensor_free_data_static(remote_sensor_data_t *sensor_data);

// frees struct members and the struct
void remote_sensor_free_data(remote_sensor_data_t *sensor_data);

void remote_sensors_update_data(cJSON *sync_json, remote_sensor_data_t *sensor_data);