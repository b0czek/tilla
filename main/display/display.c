#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "lvgl/lvgl.h"
#include "lvgl_helpers.h"

#include "drivers.h"

#define TAG "display"
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void draw(sensor_drivers_t *drivers);

void init_display(sensor_drivers_t *drivers)
{

    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, drivers, 0, NULL, 1);
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter)
{
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    static lv_disp_draw_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    // Initialize the working buffer.
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

    static lv_disp_drv_t disp_drv;

    lv_disp_drv_init(&disp_drv);
#ifdef CONFIG_USE_DISPLAY

    disp_drv.hor_res = CONFIG_LV_HOR_RES_MAX;
    disp_drv.ver_res = CONFIG_LV_VER_RES_MAX;
#endif
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;

    lv_disp_drv_register(&disp_drv);

    /* Register an input device when enabled on the menuconfig */
    // #if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    //     lv_indev_drv_t indev_drv;
    //     lv_indev_drv_init(&indev_drv);
    //     indev_drv.read_cb = touch_driver_read;
    //     indev_drv.type = LV_INDEV_TYPE_POINTER;
    //     lv_indev_drv_register(&indev_drv);
    // #endif

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .arg = pvParameter,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    draw((sensor_drivers_t *)pvParameter);

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    /* A task should NEVER return */
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

static void draw(sensor_drivers_t *drivers)
{
    // lv_theme_material_init()
    // /* Get the current screen  */

    // lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    // /*Create a Label on the currently active screen*/
    // lv_obj_t *label1 = lv_label_create(scr);

    // /*Modify the Label's text*/
    // lv_label_set_text(label1, "Hello\nworld");

    /* Align the Label to the center1
     * 0, 0 at the end means an x, y offset after alignment*/
    // lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *btn = lv_btn_create(lv_scr_act()); /*Add a button the current screen*/
    lv_obj_set_pos(btn, 10, 10);                 /*Set its position*/
    lv_obj_set_size(btn, 120, 50);               /*Set its size*/

    lv_obj_t *label = lv_label_create(btn); /*Add a label to the button*/
    lv_label_set_text_fmt(label, "%f", drivers->bme280_driver->sensors.data->data.temperature);
}

static void lv_tick_task(void *arg)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
