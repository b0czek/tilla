#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <rotary_encoder.h>

// #include "updater.h"

typedef struct display_updater_t _display_updater_t;

#define ENCODER_ENABLE CONFIG_ENCODER_ENABLE

#if ENCODER_ENABLE
typedef struct updater_encoder_t
{
    rotary_encoder_info_t info;
    TaskHandle_t queue_handler;
    QueueHandle_t event_queue;

    _display_updater_t *updater;

} updater_encoder_t;

updater_encoder_t *encoder_init(_display_updater_t *updater);
void encoder_free(updater_encoder_t *encoder);

#endif