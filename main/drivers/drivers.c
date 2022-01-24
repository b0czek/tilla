#include "drivers.h"

#include <string.h>
#include <stdlib.h>
#include <ds18b20_driver.h>
#include <bme280_driver.h>

#include <esp_log.h>
#include <vec.h>
#include <vec_helper.h>

#define TAG "drivers"

#if CONFIG_BME280_ENABLED
void bme280_parse_configs(bme280_dev_configs_t *configs, const char *sensor_configs)
{
    ESP_LOGI(TAG, "compiled sensor configs: %s", sensor_configs);
    size_t sensor_configs_len = strlen(sensor_configs) + 1;
    char *sensor_cfgs = malloc(sensor_configs_len * sizeof(char));
    strcpy(sensor_cfgs, sensor_configs);
    *(sensor_cfgs + sensor_configs_len - 1) = '\0';
    char *ptr = strtok(sensor_cfgs, ",");
    while (ptr != NULL)
    {
        bme280_dev_config_t config = {0};
        sscanf(ptr, "%hhx@%d", &(config.addr), &(config.i2c_port));
        vec_push_check vec_push(configs, config);

        ptr = strtok(NULL, ",");
    }
}

#endif

sensor_drivers_t *init_drivers()
{
    sensor_drivers_t *drivers = calloc(1, sizeof(sensor_drivers_t));

#if CONFIG_DS18B20_ENABLED
    ds18b20_config_t ds18b20_config = {
        .reading_interval = CONFIG_DS18B20_READING_INTERVAL,
        .gpio = CONFIG_DS18B20_GPIO,
        .resolution = CONFIG_DS18B20_RESOLUTION};
    ds18b20_data_t *data = calloc(1, sizeof(ds18b20_data_t));

    ds18b20_driver_init(data, &ds18b20_config);
    drivers->ds18b20_driver = data;
#endif
#if CONFIG_BME280_ENABLED

#if CONFIG_USE_I2C_BUS_0

    {
        i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = CONFIG_I2C_BUS_0_SDA,
            .scl_io_num = CONFIG_I2C_BUS_0_SCL,
            .sda_pullup_en = CONFIG_I2C_BUS_0_PULLUP_SDA,
            .scl_pullup_en = CONFIG_I2C_BUS_0_PULLUP_SCL,
            .master.clk_speed = 1000000};
        i2c_init(&i2c_config, I2C_NUM_0);

        i2c_search_devices(I2C_NUM_0);
    }

#endif
#if CONFIG_USE_I2C_BUS_1

    {
        i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = CONFIG_I2C_BUS_1_SDA,
            .scl_io_num = CONFIG_I2C_BUS_1_SCL,
            .sda_pullup_en = CONFIG_I2C_BUS_1_PULLUP_SDA,
            .scl_pullup_en = CONFIG_I2C_BUS_1_PULLUP_SCL,
            .master.clk_speed = 1000000};
        i2c_init(&i2c_config, I2C_NUM_1);

        i2c_search_devices(I2C_NUM_1);
    }

#endif
    bme280_dev_configs_t dev_configs;
    vec_init(&dev_configs);
    bme280_parse_configs(&dev_configs, CONFIG_BME280_SENSOR_CONFIGS);

    bme280_driver_t *bme280_driver = bme280_driver_init(CONFIG_BME280_READING_INTERVAL, dev_configs.data, dev_configs.length);
    vec_deinit(&dev_configs);
    drivers->bme280_driver = bme280_driver;
#endif
    return drivers;
}
