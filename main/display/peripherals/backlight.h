#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

typedef struct display_updater_t _display_updater_t;

#define BACKLIGHT_GPIO_PIN_SEL 1UL << CONFIG_DISPLAY_BACKLIGHT_GPIO
#define BACKLIGHT_GPIO CONFIG_DISPLAY_BACKLIGHT_GPIO
#define BACKLIGHT_TIMEOUT CONFIG_DISPLAY_BACKLIGHT_TIMEOUT
typedef struct display_backlight_t
{
    SemaphoreHandle_t xBacklightSemaphore;
    int16_t state;
    esp_timer_handle_t timer;

} display_backlight_t;

display_backlight_t *backlight_init();

void backlight_set_state(display_backlight_t *backlight, int16_t state);

void backlight_reset_timer(display_backlight_t *backlight);
