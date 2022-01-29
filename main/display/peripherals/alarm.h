#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct display_updater_t _display_updater_t;

#if CONFIG_ALARM_ENABLE
#define ALARM_GPIO_PIN_SEL 1UL << CONFIG_ALARM_OUTPUT_GPIO
#define ALARM_GPIO CONFIG_ALARM_OUTPUT_GPIO

typedef enum alarm_state_t
{
    // alarm is off
    DISENGAGED = 0,
    // alarm is on
    ENGAGED,
    // alarm is acknowledged and turned off
    DISARMED

} alarm_state_t;

typedef struct updater_alarm_t
{
    // queue for calls from updater sensor syncing process
    QueueHandle_t sync_queue;
    TaskHandle_t alarm_handler;

    alarm_state_t state;

    _display_updater_t *updater;

} updater_alarm_t;

updater_alarm_t *alarm_init(_display_updater_t *updater);

void alarm_disarm(updater_alarm_t *alarm);

#endif