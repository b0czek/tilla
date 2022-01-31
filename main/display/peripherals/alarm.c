#include "alarm.h"
#include "updater.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#if CONFIG_ALARM_ENABLE

#define TAG "alarm"

static void handler(void *arg);
void alarm_set_state(updater_alarm_t *alarm, alarm_state_t state);
void alarm_toggle_state(updater_alarm_t *alarm);
int find_triggered_sensor(display_updater_t *updater);
bool should_alarm_be_engaged(display_updater_t *updater);

esp_err_t alarm_gpio_init()
{
    gpio_config_t io_config = {
        .pin_bit_mask = ALARM_GPIO_PIN_SEL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t result = gpio_config(&io_config);
    if (result != 0)
    {
        ESP_LOGE(TAG, "alarm's gpio configuration failed and  resulted in %d code", result);
        return result;
    }

    result = gpio_set_level(ALARM_GPIO, !CONFIG_ALARM_TRIGGERED_STATE);
    if (result != 0)
    {
        ESP_LOGE(TAG, "alarm's gpio initial level set failed and resulted in %d code", result);
    }
    return result;
}

updater_alarm_t *alarm_init(_display_updater_t *updater)
{
    esp_err_t gpio_init_res = alarm_gpio_init();
    if (gpio_init_res != 0)
    {
        return NULL;
    }

    updater_alarm_t *alarm = calloc(1, sizeof(updater_alarm_t));
    if (!alarm)
    {
        return NULL;
    }

    alarm->state = DISENGAGED;

    alarm->updater = updater;

    // create queue for updater thread to send items to when update of any remote sensor occurs
    // value sent in queue is index of remote sensor that was updated
    alarm->sync_queue = xQueueCreate(updater->remote_sensors_count, sizeof(int));

    xTaskCreate(handler, "alarm_handler", 2048, alarm, 0, &alarm->alarm_handler);

    return alarm;
}

static void handler(void *arg)
{
    updater_alarm_t *alarm = arg;
    display_updater_t *updater = alarm->updater;

    while (1)
    {
        int sensor_idx = 0;
        // receive from queue indefinitely
        if (xQueueReceive(alarm->sync_queue, &sensor_idx, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        // try to lock on updater
        if (xSemaphoreTake(updater->xUpdaterSemaphore, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        remote_sensor_data_t *remote_sensor = updater->remote_sensors + sensor_idx;
        // updated sensor must not be in error state
        if (remote_sensor->error)
        {
            xSemaphoreGive(updater->xUpdaterSemaphore);
            continue;
        }

        bool sensor_alarm_triggered = false;

        // iterate through all of the fields
        for (int i = 0; i < remote_sensor->fields_count; i++)
        {
            // check if any of the fields triggers the alarm
            remote_sensor_field_t *field = remote_sensor->fields + i;
            if (field->current_value > field->alarm_upper_threshold || field->current_value < field->alarm_lower_threshold)
            {
                sensor_alarm_triggered = true;
                break;
            }
        }
        remote_sensor->alarm_triggered = sensor_alarm_triggered;

        bool desired_state = should_alarm_be_engaged(updater);
        alarm_state_t prev_state = alarm->state;

        // if alarm was previously disarmed
        if (alarm->state == DISARMED)
        {
            //                          if new sensor's alarm was triggered           or there is no active alarms anymore
            alarm_state_t new_state = (sensor_alarm_triggered ? ENGAGED : (desired_state == DISENGAGED ? DISENGAGED : DISARMED));
            // if state changed
            if (new_state != prev_state)
            {
                alarm_set_state(alarm, new_state);
            }
        }
        // if alarm was not disarmed, check if it's state should change
        else if (desired_state != alarm->state)
        {
            alarm_toggle_state(alarm);
        }

        // if state changed to engaged, switch currently displayed sensor to the triggered one
        if (prev_state != alarm->state && alarm->state == ENGAGED)
        {
            // if currently handled sensor's alarm was triggered, set it as active, otherwise use firstly find triggered sensor
            updater->active_remote_sensor = sensor_alarm_triggered ? sensor_idx : find_triggered_sensor(updater);
            updater->layout = layout_reload(updater->layout, updater->remote_sensors + updater->active_remote_sensor, updater->xGuiSemaphore);
        }

        xSemaphoreGive(updater->xUpdaterSemaphore);
    }
}

void alarm_set_state(updater_alarm_t *alarm, alarm_state_t state)
{
    alarm->state = state;
    if (!xPortInIsrContext())
    {
        ESP_LOGE(TAG, "alarm changed state to %d", alarm->state);
    }
    else
    {
        ets_printf("alarm changed state to %d", alarm->state);
    }
    // xnor
    // ----------------------------------------
    // | triggered_state |    state   | output|
    // |        1        |    0(0)    |   0   | disengaged
    // |        1        |    0(1)    |   1   | engaged
    // |        1        |    1(0)    |   0   | disarmed
    // |        0        |    0(0)    |   1   |
    // |        0        |    0(1)    |   0   |
    // |        0        |    1(0)    |   1   |
    // ----------------------------------------
    //                                                     get rightmost bit
    int8_t output_state = CONFIG_ALARM_TRIGGERED_STATE == (alarm->state & 1);
    // set the state on gpio
    gpio_set_level(ALARM_GPIO, output_state);
}
// switch state from disarmed or disengaged to engaged and from engaged to disengaged
void alarm_toggle_state(updater_alarm_t *alarm)
{
    alarm_set_state(alarm, !(alarm->state & 1));
}

void alarm_disarm(updater_alarm_t *alarm)
{
    // function only works if alarm is engaged
    if (alarm->state != ENGAGED)
    {
        return;
    }

    alarm_set_state(alarm, DISARMED);
}

// finds and returns index of firstly found sensor with alarm triggered set to true
// returns -1 if it was not found
int find_triggered_sensor(display_updater_t *updater)
{
    for (int i = 0; i < updater->remote_sensors_count; i++)
    {
        if ((updater->remote_sensors + i)->alarm_triggered)
        {
            return i;
        }
    }
    return -1;
}

bool should_alarm_be_engaged(display_updater_t *updater)
{
    return find_triggered_sensor(updater) != -1;
}

#endif
