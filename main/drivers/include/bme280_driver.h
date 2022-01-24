#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_err.h>
#include <bme280.h>
#include "driver/i2c.h"
#include <vec.h>

#define BME280_INIT_RETRY_COUNT 5

typedef struct bme280_dev bme280_dev_t;
typedef struct bme280_data bme280_data_t;

typedef struct
{
    // i2c address. 0x76, 0x77
    uint8_t addr;
    i2c_port_t i2c_port;
} bme280_dev_config_t;
typedef vec_t(bme280_dev_config_t) bme280_dev_configs_t;

typedef struct
{
    bme280_dev_t dev;
    bme280_data_t data;
    int8_t error;
    uint8_t settings;
    bme280_dev_config_t config;

} bme280_sensor_t;
typedef vec_t(bme280_sensor_t) bme280_sensors_t;

typedef struct
{
    bme280_sensors_t sensors;
    int8_t error;
    uint32_t reading_interval;
    TaskHandle_t xHandle;
    SemaphoreHandle_t xSemaphore;
} bme280_driver_t;

void i2c_init(i2c_config_t *config, i2c_port_t i2c_num);
void i2c_search_devices();

bme280_driver_t *bme280_driver_init(uint32_t reading_interval, bme280_dev_config_t *dev_configs, int cfg_len);
void delay_us(uint32_t period, void *intf_ptr);
