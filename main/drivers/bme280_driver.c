#include "bme280_driver.h"
#include <bme280.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/task.h"

#include <esp_err.h>

#define DRIVER_TAG "bme280_driver"

// round to two decimal places
double round_double(double var)
{
    double value = (int)(var * 100 + .5);
    return (double)value / 100;
}

static int vec_push_check(int r, int x)
{
    return r;
}

void i2c_init(i2c_config_t *config, i2c_port_t i2c_num)
{

    i2c_param_config(i2c_num, config);
    i2c_driver_install(i2c_num, I2C_MODE_MASTER, 0, 0, 0);
}

void i2c_search_devices(i2c_port_t i2c_num)
{
    ESP_LOGI(DRIVER_TAG, "i2c scan bus %d: ", i2c_num);

    for (uint8_t i = 1; i < 127; i++)
    {
        int ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 100 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            ESP_LOGI(DRIVER_TAG, "Found device at: 0x%2x", i);
        }
    }
}

static int8_t bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    bme280_dev_config_t *dev_config = (bme280_dev_config_t *)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_config->addr << 1) | I2C_MASTER_WRITE, true);

    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, reg_data, len, true);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(dev_config->i2c_port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    // without this print statement it does not work for some reason, i guess it's timing issue???
    printf("%d", res);

    return res;
}

static int8_t bme280_i2c_read(const uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    bme280_dev_config_t *dev_config = (bme280_dev_config_t *)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_config->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    // i2c_master_stop(cmd);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_config->addr << 1) | I2C_MASTER_READ, true);

    if (len > 1)
    {
        i2c_master_read(cmd, reg_data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(dev_config->i2c_port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    printf("%d", res);
    // ESP_LOGE(DRIVER_TAG, "read on %d bus, address 0x%.2x - result %d\n", dev_config->i2c_port, dev_config->addr, res);
    // if (res != ESP_OK)
    // {
    // ESP_LOGE(DRIVER_TAG, "I2C READ RESULTED IN %d", res);
    // }
    // return res == 0 ? BME280_OK : BME280_E_COMM_FAIL;
    return res;
}

void delay_us(uint32_t period, void *intf_ptr)
{
    vTaskDelay(period / 1000 / portTICK_PERIOD_MS);
    return;
}

void print_sensor_data(bme280_sensor_t *sensor)
{
    bme280_data_t *comp_data = &sensor->data;
    printf("|0x%.2x| error: %d, %0.2f, %0.2f, %0.2f\r\n", sensor->config.addr, sensor->error, comp_data->temperature, comp_data->pressure, comp_data->humidity);
}

static void task_bme280_normal_mode(void *sensor_ptr)
{
    bme280_driver_t *driver = (bme280_driver_t *)sensor_ptr;

    ESP_LOGI(DRIVER_TAG, "bme280 sensors count: %d", driver->sensors.length);
    for (;;)
    {
        for (int i = 0; i < driver->sensors.length; i++)
        {
            bme280_sensor_t *sensor = (driver->sensors.data) + i;

            sensor->error = bme280_get_sensor_data(BME280_ALL, &(sensor->data), &(sensor->dev));
            sensor->data.temperature = round_double(sensor->data.temperature);
            sensor->data.humidity = round_double(sensor->data.humidity);
            sensor->data.pressure = (int)sensor->data.pressure;
            // print_sensor_data(sensor);
        }
        xSemaphoreGive(driver->xSemaphore);
        delay_us(driver->reading_interval * 1000, NULL);
    }
}

int8_t bme280_device_init(bme280_sensor_t *sensor)
{
    int8_t result = 0;

    sensor->dev.intf_ptr = &(sensor->config);
    sensor->dev.intf = BME280_I2C_INTF;
    sensor->dev.read = bme280_i2c_read;
    sensor->dev.write = bme280_i2c_write;
    sensor->dev.delay_us = delay_us;

    result += bme280_init(&(sensor->dev));
    ESP_LOGI(DRIVER_TAG, "initialization of bme280 sensor |%.2x@%d| resulted in code %d", sensor->config.addr, sensor->config.i2c_port, result);

    sensor->dev.settings.osr_h = BME280_OVERSAMPLING_1X;
    sensor->dev.settings.osr_p = BME280_OVERSAMPLING_16X;
    sensor->dev.settings.osr_t = BME280_OVERSAMPLING_2X;
    sensor->dev.settings.filter = BME280_FILTER_COEFF_16;
    sensor->dev.settings.standby_time = BME280_STANDBY_TIME_62_5_MS;
    sensor->settings = BME280_OSR_PRESS_SEL |
                       BME280_OSR_TEMP_SEL |
                       BME280_OSR_HUM_SEL |
                       BME280_FILTER_SEL |
                       BME280_STANDBY_SEL;
    result += bme280_set_sensor_settings(sensor->settings, &(sensor->dev));
    result += bme280_set_sensor_mode(BME280_NORMAL_MODE, &(sensor->dev));

    return result;
}

bme280_driver_t *bme280_driver_init(uint32_t reading_interval, bme280_dev_config_t *dev_configs, int cfg_len)
{

    bme280_driver_t *driver = calloc(1, sizeof(bme280_driver_t));
    driver->reading_interval = reading_interval;
    vec_init(&driver->sensors);
    int retries = 0;

    for (int i = 0; i < cfg_len; i++)
    {
        bme280_sensor_t sensor = {0};
        memcpy(&sensor.config, dev_configs + i, sizeof(bme280_dev_config_t));
        vec_push_check vec_push(&(driver->sensors), sensor);
        do
        {
            sensor.error = bme280_device_init(driver->sensors.data + i);
            retries++;
        } while (sensor.error != 0 && retries < BME280_INIT_RETRY_COUNT);
    }

    driver->xSemaphore = xSemaphoreCreateMutex();

    xTaskCreate(task_bme280_normal_mode, "bme280_normal_mode", 2048, driver, tskIDLE_PRIORITY + 2, &driver->xHandle);
    return driver;
}

esp_err_t bme280_driver_free(bme280_driver_t *sensor)
{
    return ESP_OK;
}