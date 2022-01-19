#pragma once
#include "../fetcher/fetcher.h"
#include "display.h"
#include "layout.h"
#include "encoder.h"

#include "remote_sensor.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// how much time to wait between attempting to query info endpoint
#define UPDATER_TIME_BETWEEN_INIT_ATTEMPTS 20000

typedef struct display_updater_t
{
    fetcher_client_t *client;
    SemaphoreHandle_t xUpdaterSemaphore;
    TaskHandle_t xHandle;

    SemaphoreHandle_t xGuiSemaphore;

    remote_sensor_data_t *remote_sensors;
    int remote_sensors_count;
    // index of remote sensor currently displayed on the screen

    int active_remote_sensor;

    updater_layout_t *layout;

    updater_encoder_t *encoder;
} display_updater_t;

display_updater_t *
display_updater_init(SemaphoreHandle_t xGuiSemaphore);
