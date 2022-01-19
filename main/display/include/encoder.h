
#pragma once
#include <rotary_encoder.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct display_updater_t _display_updater_t;

typedef struct updater_encoder_t
{
    rotary_encoder_info_t info;
    QueueHandle_t event_queue;
    TaskHandle_t queue_handler;

    _display_updater_t *updater;
} updater_encoder_t;
updater_encoder_t *encoder_init(_display_updater_t *updater);
