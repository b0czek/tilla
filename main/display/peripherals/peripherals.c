#include <rotary_encoder.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_log.h>

#include "peripherals.h"
#include "updater.h"

updater_peripherals_t *peripherals_init(display_updater_t *updater)
{
    updater_peripherals_t *peripherals = calloc(1, sizeof(updater_encoder_t));
    if (!peripherals)
    {
        return NULL;
    }
    peripherals->updater = updater;

#if ENCODER_ENABLE
    peripherals->encoder = encoder_init(updater);
#endif

#if ALARM_ENABLE
    peripherals->alarm = alarm_init(updater);
#endif

    return peripherals;
}
