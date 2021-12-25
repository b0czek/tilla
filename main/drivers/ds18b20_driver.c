#include "ds18b20_driver.h"
#include <esp_err.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/task.h>

#define DATA_CALLOC_CHECK(data_ptr, ref) \
    if (ref == NULL)                     \
    {                                    \
        ds18b20_driver_free(data_ptr);   \
        return ESP_ERR_NO_MEM;           \
    }

// vec_push returns '(int, int)' for some reason and i cant get it to work
// so i use the macro output as function parameter
// vec_push_check vec_push(v, val)
static int vec_push_check(int r, int x)
{
    return r;
}

static void vTaskReadDevices(void *data)
{
    ds18b20_data_t *ds_data = data;
    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        ds18b20_convert_all(ds_data->owb);
        // all devices have the same resolution, so their conversion time will be the same as well
        ds18b20_wait_for_conversion(ds_data->devices.data->device);

        for (int i = 0; i < ds_data->devices.length; i++)
        {
            ds18b20_device_t *device = (ds_data->devices.data + i);
            device->error = ds18b20_read_temp(device->device, &device->reading);
            // printf("sensor %d, error: %d, value: %f\n", i, device->error, device->reading);
        }

        vTaskDelayUntil(&last_wake_time, ds_data->config->reading_interval / portTICK_PERIOD_MS);
    }
}

esp_err_t ds18b20_driver_free(ds18b20_data_t *data)
{
    if (data->xHandle != NULL)
    {
        vTaskDelete(data->xHandle);
    }
    free(data->config);
    owb_uninitialize(data->owb);

    free(data->owb_driver);

    for (int i = 0; i < data->devices.length; i++)
    {
        ds18b20_device_t *dev = (data->devices.data + i);
        ds18b20_free(&dev->device);
    }
    vec_deinit(&data->devices);

    if (data->config)
    {
        free(data->config);
    }
    free(data);
    return ESP_OK;
}

esp_err_t ds18b20_driver_init(ds18b20_data_t *data, ds18b20_config_t *config)
{
    data->error = true;

    // copy config
    data->config = calloc(1, sizeof(ds18b20_config_t));
    DATA_CALLOC_CHECK(data, data->config);
    memcpy(data->config, config, sizeof(ds18b20_config_t));
    // initialize driver
    data->owb_driver = calloc(1, sizeof(owb_rmt_driver_info));
    DATA_CALLOC_CHECK(data, data->owb_driver);

    data->owb = owb_rmt_initialize(data->owb_driver, config->gpio, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(data->owb, true); // enable CRC check for ROM code

    vec_init(&(data->devices));
    int num_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(data->owb, &search_state, &found);

    while (found)
    {

        DS18B20_Info *ds_info = ds18b20_malloc();
        ds18b20_init(ds_info, data->owb, search_state.rom_code);
        ds18b20_use_crc(ds_info, true); // enable CRC check on all reads
        ds18b20_set_resolution(ds_info, config->resolution);

        ds18b20_device_t device = {
            .device = ds_info,
            .error = 0,
            .reading = 0.0,
        };
        vec_push_check vec_push(&(data->devices), device);
        // vec_insert(&(data->devices), data->devices.length, device);

        char rom_code_s[17];
        owb_string_from_rom_code((data->devices.data + num_devices)->device->rom_code, rom_code_s, sizeof(rom_code_s));
        printf("  %d : 0x%s\n", num_devices, rom_code_s);

        ++num_devices;
        owb_search_next(data->owb, &search_state, &found);
    }
    printf("num of devices: %d\n", num_devices);
    // Check for parasitic-powered devices
    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(data->owb, &parasitic_power);
    if (parasitic_power)
    {
        printf("Parasitic-powered devices detected");
    }

    // In parasitic-power mode, devices cannot indicate when conversions are complete,
    // so waiting for a temperature conversion must be done by waiting a prescribed duration
    owb_use_parasitic_power(data->owb, parasitic_power);

    data->error = false;

    if (num_devices > 0)
    {
        xTaskCreate(vTaskReadDevices, "ds18b20_read", STACK_SIZE, data, tskIDLE_PRIORITY + 2, &data->xHandle);
    }

    return ESP_OK;
}
