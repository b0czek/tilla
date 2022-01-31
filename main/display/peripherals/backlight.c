#include "backlight.h"
#include "updater.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>

static void backlight_timeout_cb(void *arg);

display_backlight_t *backlight_init()
{
    display_backlight_t *backlight = calloc(1, sizeof(display_backlight_t));
    if (!backlight)
    {
        return NULL;
    }

    gpio_config_t io_config = {
        .pin_bit_mask = BACKLIGHT_GPIO_PIN_SEL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_config);

    const esp_timer_create_args_t timer_args = {
        .callback = backlight_timeout_cb,
        .arg = backlight,
        .name = "display_timeout"

    };
    esp_timer_create(&timer_args, &backlight->timer);

    backlight->xBacklightSemaphore = xSemaphoreCreateMutex();

    // start with display on
    backlight_set_state(backlight, 1);

    return backlight;
}

void backlight_set_state(display_backlight_t *backlight, int16_t state)
{
    backlight->state = state;
    gpio_set_level(BACKLIGHT_GPIO, state);

    if (state == 0)
    {
        esp_timer_stop(backlight->timer);
    }
    else
    {
        backlight_reset_timer(backlight);
    }
}

void backlight_reset_timer(display_backlight_t *backlight)
{
    if (esp_timer_is_active(backlight->timer))
    {
        esp_timer_stop(backlight->timer);
    }
    esp_timer_start_once(backlight->timer, BACKLIGHT_TIMEOUT * 1000);
}

static void backlight_timeout_cb(void *arg)
{
    display_backlight_t *backlight = arg;

    if (xSemaphoreTake(backlight->xBacklightSemaphore, portMAX_DELAY) == pdTRUE)
    {

        backlight_set_state(backlight, 0);
        xSemaphoreGive(backlight->xBacklightSemaphore);
    }
}
