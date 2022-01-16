#pragma once
#include "stdint.h"
#include <cJSON.h>

typedef struct remote_sensor_data_t
{
    char *device_name;
    char *sensor_name;
    char *sensor_type;
    char *remote_sensor_uuid;
    int32_t polling_interval;
    int32_t max_sample_age;
    int32_t sample_count;
    uint32_t last_update_timestamp;
} remote_sensor_data_t;

// initializes data in passed struct, although there are still some mallocs needed for strings
int remote_sensor_init_data_static(cJSON *sensor_json, remote_sensor_data_t *sensor_data);

// initializes data and returns malloced struct
remote_sensor_data_t *remote_sensor_init_data(cJSON *sensor_json);

// frees strings in struct, but not the struct itself
void remote_sensor_free_data_static(remote_sensor_data_t *sensor_data);

// frees strings in struct and the struct
void remote_sensor_free_data(remote_sensor_data_t *sensor_data);