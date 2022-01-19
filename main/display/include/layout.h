#pragma once
#include "lvgl.h"
#include "remote_sensor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef struct updater_field_objects_t
{
    lv_chart_series_t *chart_series;
    lv_obj_t *field;

} updater_field_objects_t;

typedef struct updater_layout_t
{
    lv_obj_t *parent;
    lv_obj_t *sensor_name;
    lv_obj_t *chart;
    updater_field_objects_t *field_objects;
    uint8_t field_objects_count;

} updater_layout_t;

updater_layout_t *layout_init(remote_sensor_data_t *remote_sensor, SemaphoreHandle_t xGuiSemaphore);

void layout_free(updater_layout_t *layout, SemaphoreHandle_t xGuiSemaphore);

int layout_set_error(updater_layout_t *layout, remote_sensor_data_t *remote_sensor, lv_color_t color, SemaphoreHandle_t xGuiSemaphore);

int layout_update_data(updater_layout_t *layout, remote_sensor_data_t *remote_sensor, SemaphoreHandle_t xGuiSemaphore);
