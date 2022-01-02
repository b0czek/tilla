#include "drivers.h"

#include <stdlib.h>
#include <ds18b20_driver.h>
#include <bme280_driver.h>

sensor_drivers_t *init_drivers()
{
    sensor_drivers_t *drivers = calloc(1, sizeof(sensor_drivers_t));

#ifdef CONFIG_DS18B20_ENABLED
    ds18b20_config_t ds18b20_config = {
        .reading_interval = CONFIG_DS18B20_READING_INTERVAL,
        .gpio = CONFIG_DS18B20_GPIO,
        .resolution = CONFIG_DS18B20_RESOLUTION};
    ds18b20_data_t *data = calloc(1, sizeof(ds18b20_data_t));

    ds18b20_driver_init(data, &ds18b20_config);
    drivers->ds18b20_driver = data;
#endif
#ifdef CONFIG_BME280_ENABLED

    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 16,
        .scl_io_num = 4,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000};
    i2c_init(&i2c_config, I2C_NUM_0);

    i2c_search_devices(I2C_NUM_0);
    bme280_dev_config_t dev_configs[] = {
        {.addr = 0x76,
         .i2c_port = I2C_NUM_0},
        {.addr = 0x77,
         .i2c_port = I2C_NUM_0},
    };
    bme280_driver_t *bme280_driver = bme280_driver_init(1000, dev_configs, sizeof(dev_configs) / sizeof(bme280_dev_config_t));

    drivers->bme280_driver = bme280_driver;
#endif
    return drivers;
}
