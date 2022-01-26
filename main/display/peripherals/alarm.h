#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct display_updater_t _display_updater_t;

#if CONFIG_ALARM_ENABLE
#define ALARM_GPIO_PIN_SEL 1UL << CONFIG_ALARM_OUTPUT_GPIO
#define ALARM_GPIO CONFIG_ALARM_OUTPUT_GPIO

typedef struct updater_alarm_t
{
    // queue for calls from updater sensor syncing process
    QueueHandle_t sync_queue;
    TaskHandle_t alarm_handler;

    bool is_engaged;

    _display_updater_t *updater;

} updater_alarm_t;

updater_alarm_t *alarm_init(_display_updater_t *updater);

#endif