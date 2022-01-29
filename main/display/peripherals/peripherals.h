#pragma once

#include "backlight.h"

#define ENCODER_ENABLE CONFIG_ENCODER_ENABLE

#if ENCODER_ENABLE
#include "encoder.h"
#endif

#define ALARM_ENABLE CONFIG_ALARM_ENABLE
#if ALARM_ENABLE
#include "alarm.h"
#endif

typedef struct display_updater_t _display_updater_t;

typedef struct updater_peripherals_t
{
    display_backlight_t *backlight;

#if ENCODER_ENABLE
    updater_encoder_t *encoder;
#endif
#if ALARM_ENABLE
    updater_alarm_t *alarm;
#endif

    _display_updater_t *updater;

} updater_peripherals_t;

updater_peripherals_t *peripherals_init(_display_updater_t *updater);
