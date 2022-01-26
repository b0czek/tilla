#include <esp_err.h>
#include "encoder.h"
#include "updater.h"

#if ENCODER_ENABLE

#define TAG "encoder"

static void handler(void *arg);

updater_encoder_t *encoder_init(display_updater_t *updater)
{
    esp_err_t init_res = gpio_install_isr_service(0);
    if (init_res != 0)
    {
        return NULL;
    }

    updater_encoder_t *encoder = calloc(1, sizeof(updater_encoder_t));
    if (!encoder)
    {
        return NULL;
    }

    encoder->updater = updater;
    encoder->event_queue = rotary_encoder_create_queue();

    rotary_encoder_init(&encoder->info, CONFIG_ENCODER_GPIO_A, CONFIG_ENCODER_GPIO_B);
    rotary_encoder_set_queue(&encoder->info, encoder->event_queue);

    xTaskCreate(handler, "encoder_handler", 2048, encoder, 0, &encoder->queue_handler);

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

static void handler(void *arg)
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