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

    int16_t alarm_lower_threshold;
    int16_t alarm_upper_threshold;

    double current_value;
    int16_t *values;

} remote_sensor_field_t;

typedef struct remote_sensor_data_t
{
    // data received in info query
    char *device_name;
    char *sensor_name;
    char *sensor_type;
    char *remote_sensor_uuid;
    // polling interval is implemented in a way that every second update_counter is incremented
    // and if it reaches or exceeds polling_interval's seconds setting, query is made and counter is back to 0.
    // * NOTE: despite being held in millisecond value, the fraction part of a second is not taken into consideration
    // * Moreover, queries for each sensor are done separately and sequentially, so it is not guaranteed,
    // * that the interval will be strictly obeyed.
    int32_t polling_interval;
    int32_t max_sample_age;
    int32_t sample_count;

    remote_sensor_field_t *fields;
    int8_t fields_count;

    bool error;
    bool device_online;

    int32_t update_counter;
    // timestamp of last sample received in sync query
    uint64_t last_update_timestamp;

    bool alarm_triggered;

} remote_sensor_data_t;

// initializes data in passed struct, although there are still some mallocs needed for struct members
int remote_sensor_init_data_static(cJSON *sensor_json, remote_sensor_data_t *sensor_data);

// initializes data and returns malloced struct
remote_sensor_data_t *remote_sensor_init_data(cJSON *sensor_json);

// frees struct members, but not the struct itself
void remote_sensor_free_data_static(remote_sensor_data_t *sensor_data);

// frees struct members and the struct
void remote_sensor_free_data(remote_sensor_data_t *sensor_data);

// sets error value on sensor_data, and if it is true, pushes empty value on all of the sensor fields
void remote_sensor_set_error_state(remote_sensor_data_t *sensor_data, bool error);

void remote_sensor_update_data(cJSON *sync_json, remote_sensor_data_t *sensor_data);