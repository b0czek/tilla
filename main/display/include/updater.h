#pragma once
#include "../fetcher/fetcher.h"
#include "display.h"
#include "remote_sensor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// how much time to wait between attempting to query info endpoint
#define UPDATER_TIME_BETWEEN_INIT_ATTEMPTS 20000

typedef struct display_updater_t
{
    fetcher_client_t *client;
    SemaphoreHandle_t xGuiSemaphore;
    TaskHandle_t xHandle;
    remote_sensor_data_t *remote_sensors;
    int remote_sensors_count;

} display_updater_t;

display_updater_t *
display_updater_init(SemaphoreHandle_t xGuiSemaphore);
