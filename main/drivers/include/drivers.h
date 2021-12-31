#pragma once

#include <ds18b20_driver.h>
#include <bme280_driver.h>

typedef struct
{
    bme280_driver_t *bme280_driver;
    ds18b20_data_t *ds18b20_driver;
} sensor_drivers_t;

sensor_drivers_t *init_drivers();
