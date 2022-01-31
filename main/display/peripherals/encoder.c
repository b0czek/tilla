#include <esp_err.h>
#include <esp_log.h>
#include "encoder.h"
#include "updater.h"
#include "backlight.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#if ENCODER_ENABLE

#define TAG "encoder"
#define BUTTON_GPIO CONFIG_ENCODER_GPIO_BUTTON

static void encoder_handler(void *arg);
static void button_isr_handler(void *arg);

void init_button_gpio(gpio_num_t gpio_num)
{
    gpio_pad_select_gpio(gpio_num);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_intr_type(gpio_num, GPIO_INTR_POSEDGE);
}

updater_encoder_t *encoder_init(display_updater_t *updater)
{
    updater_encoder_t *encoder = calloc(1, sizeof(updater_encoder_t));
    if (!encoder)
    {
        return NULL;
    }

    encoder->updater = updater;
    encoder->event_queue = rotary_encoder_create_queue();

    rotary_encoder_init(&encoder->info, CONFIG_ENCODER_GPIO_A, CONFIG_ENCODER_GPIO_B);
    rotary_encoder_set_queue(&encoder->info, encoder->event_queue);

    init_button_gpio(BUTTON_GPIO);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, encoder);

    xTaskCreate(encoder_handler, "encoder_handler", 2048, encoder, 0, &encoder->queue_handler);

    return encoder;
}

void encoder_free(updater_encoder_t *encoder)
{
    if (encoder->queue_handler)
    {

        vTaskDelete(encoder->queue_handler);
    }

    rotary_encoder_uninit(&encoder->info);
    vQueueDelete(encoder->event_queue);

    free(encoder);
}

static void button_isr_handler(void *arg)
{
    updater_encoder_t *encoder = arg;
    display_backlight_t *backlight = encoder->updater->peripherals->backlight;

#if CONFIG_ALARM_ENABLE
    updater_alarm_t *alarm = encoder->updater->peripherals->alarm;
    // if alarm is enabled and engaged, button click should disarm it
    if (alarm->state == ENGAGED)
    {
        alarm_disarm(alarm);
        return;
    }
#endif
    BaseType_t xTaskWoken = 0;
    if (xSemaphoreTakeFromISR(backlight->xBacklightSemaphore, &xTaskWoken) == pdTRUE)
    {

        backlight_set_state(backlight, (backlight->state + 1) % 2);
        xSemaphoreGiveFromISR(backlight->xBacklightSemaphore, &xTaskWoken);
    }
}

static void encoder_handler(void *arg)
{
    updater_encoder_t *encoder = arg;
    display_updater_t *updater = encoder->updater;

    while (1)
    {
        rotary_encoder_event_t event = {0};
        if (xQueueReceive(encoder->event_queue, &event, 1000 / portTICK_PERIOD_MS) != pdTRUE)
        {
            continue;
        }
        if (xSemaphoreTake(updater->xUpdaterSemaphore, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        display_backlight_t *backlight = updater->peripherals->backlight;
        if (xSemaphoreTake(backlight->xBacklightSemaphore, portMAX_DELAY) == pdTRUE)
        {

            // turn on the screen if encoder was turned
            if (backlight->state == 0)
            {
                backlight_set_state(backlight, 1);
            }
            // or reset the timer if it was on already
            else
            {
                backlight_reset_timer(backlight);
            }
            xSemaphoreGive(backlight->xBacklightSemaphore);
        }

        if (updater->remote_sensors_count > 1)
        {
            int direction = event.state.direction == ROTARY_ENCODER_DIRECTION_CLOCKWISE ? 1 : -1;
            // if encoder turned left and remote sensor is at index 0, set active sensor to the last one
            if (direction == -1 && updater->active_remote_sensor == 0)
            {
                updater->active_remote_sensor = updater->remote_sensors_count - 1;
            }
            else
            {
                updater->active_remote_sensor = (updater->active_remote_sensor + direction) % updater->remote_sensors_count;
            }

            remote_sensor_data_t *active_sensor = updater->remote_sensors + updater->active_remote_sensor;
            updater->layout = layout_reload(updater->layout, active_sensor, updater->xGuiSemaphore);
        }

        xSemaphoreGive(updater->xUpdaterSemaphore);
    }
}

#endif