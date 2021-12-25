#pragma once

#include <ds18b20.h>
#include <owb.h>
#include <vec.h>
#include <freertos/task.h>
#include <esp_err.h>

#define STACK_SIZE 2048

typedef struct
{
    uint32_t reading_interval;
    uint8_t gpio;
    DS18B20_RESOLUTION resolution;

} ds18b20_config_t;

typedef struct
{
    DS18B20_Info *device;
    DS18B20_ERROR error;
    float reading;
} ds18b20_device_t;

typedef vec_t(ds18b20_device_t) ds18b20_devices_t;

typedef struct
{
    owb_rmt_driver_info *owb_driver;
    OneWireBus *owb;
    ds18b20_devices_t devices;
    ds18b20_config_t *config;
    TaskHandle_t xHandle;
    bool error;

} ds18b20_data_t;

esp_err_t ds18b20_driver_free(ds18b20_data_t *data);
esp_err_t ds18b20_driver_init(ds18b20_data_t *data, ds18b20_config_t *config);
