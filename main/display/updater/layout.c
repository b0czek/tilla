#include "layout.h"
#include "updater.h"

#include <esp_log.h>

#define CHART_HEIGHT 160
#define MARGIN 20
#define FULL_WIDTH CONFIG_LV_HOR_RES_MAX - MARGIN * 2
#define FULL_HEIGHT CONFIG_LV_VER_RES_MAX - MARGIN * 2

#define OUTLINE_WIDTH 3

#define VOID
/**
 * @brief macro used for taking a samephore if it is not null and returning passed value when that fails, 
 * in case of void function use `xSemaphoreTakeIfNotNullOrFail(xSemaphore, VOID);` for better readability
 */
#define xSemaphoreTakeIfNotNullOrFail(xSemaphore, returnValueOnFail) \
    if (xSemaphore)                                                  \
    {                                                                \
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) != pdTRUE)     \
        {                                                            \
            return returnValueOnFail;                                \
        }                                                            \
    }

#define xSemaphoreGiveIfNotNull(xSemaphore) \
    if (xSemaphore)                         \
    {                                       \
        xSemaphoreGive(xSemaphore);         \
    }

static void create_parent_obj(updater_layout_t *layout)
{
    layout->parent = lv_obj_create(lv_scr_act());
    lv_obj_set_align(layout->parent, LV_ALIGN_CENTER);
    lv_obj_set_size(layout->parent, CONFIG_LV_HOR_RES_MAX - OUTLINE_WIDTH * 2, CONFIG_LV_VER_RES_MAX - OUTLINE_WIDTH * 2);
    lv_obj_set_layout(layout->parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(layout->parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(layout->parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_outline_width(layout->parent, OUTLINE_WIDTH, 0);
}

static void create_sensor_name_label(updater_layout_t *layout, remote_sensor_data_t *remote_sensor_data)
{
    layout->sensor_name = lv_label_create(layout->parent);
    lv_obj_set_width(layout->sensor_name, FULL_WIDTH);
    lv_obj_set_style_text_align(layout->sensor_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(layout->sensor_name, &lv_font_montserrat_30, 0);
    lv_label_set_text_fmt(layout->sensor_name, "%s, %s", remote_sensor_data->sensor_name, remote_sensor_data->device_name);
}

static void create_chart(updater_layout_t *layout, remote_sensor_data_t *remote_sensor)
{
    layout->chart = lv_chart_create(layout->parent);

    // set size and placement, - subtract margin for axis ticks
    lv_obj_set_size(layout->chart, FULL_WIDTH - MARGIN, CHART_HEIGHT);
    // set chart type to line
    lv_chart_set_type(layout->chart, LV_CHART_TYPE_LINE);

    // set line styling
    lv_obj_set_style_line_width(layout->chart, 5, LV_PART_ITEMS);
    lv_obj_set_style_size(layout->chart, 5, LV_PART_INDICATOR);
    // and point count
    lv_chart_set_point_count(layout->chart, remote_sensor->sample_count);
    lv_chart_set_div_line_count(layout->chart, 5, 5);
    for (int i = 0; i < remote_sensor->fields_count; i++)
    {
        remote_sensor_field_t *field = remote_sensor->fields + i;

        (layout->field_objects + i)->chart_series = lv_chart_add_series(layout->chart, lv_color_hex(field->color), field->priority);
        lv_chart_set_range(layout->chart, field->priority, field->range_min, field->range_max);
        lv_chart_set_axis_tick(layout->chart, field->priority, 0, 0, 5, 1, true, 50);

        lv_chart_set_ext_y_array(layout->chart, (layout->field_objects + i)->chart_series, field->values);
    }
}

static void create_field_labels(updater_layout_t *layout, remote_sensor_data_t *remote_sensor)
{
    for (int i = 0; i < remote_sensor->fields_count; i++)
    {
        (layout->field_objects + i)->field = lv_label_create(layout->parent);
        lv_obj_t *field = (layout->field_objects + i)->field;

        lv_obj_set_style_text_font(field, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_align(field, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_set_style_outline_width(field, OUTLINE_WIDTH, 0);
        lv_obj_set_style_outline_color(field, lv_color_hex((remote_sensor->fields + i)->color), 0);
        lv_obj_set_style_radius(field, 5, 0);
        lv_obj_set_size(field, FULL_WIDTH, 32);
    }
}

updater_layout_t *layout_init(remote_sensor_data_t *remote_sensor, SemaphoreHandle_t xGuiSemaphore)
{
    // lock the gui
    xSemaphoreTakeIfNotNullOrFail(xGuiSemaphore, NULL);

    // allocate memory for layout struct
    updater_layout_t *layout = malloc(sizeof(updater_layout_t));

    // if allocation fails, unlock gui and return null
    if (layout == NULL)
    {
        xSemaphoreGiveIfNotNull(xGuiSemaphore);
        return NULL;
    }
    // if there are more than 1 field
    if (remote_sensor->fields_count > 0)
    {
        // allocate memory for pointers to the series
        layout->field_objects = malloc(sizeof(updater_field_objects_t) * remote_sensor->fields_count);
        // if memory could not be allocated
        if (!layout->field_objects)
        {
            // free the layout memory
            free(layout);
            // unlock gui
            xSemaphoreGiveIfNotNull(xGuiSemaphore);
            return NULL;
        }
        layout->field_objects_count = remote_sensor->fields_count;
    }
    // if there aren't, set pointer to null because it won't be used either way
    else
    {
        layout->field_objects = NULL;
    }

    create_parent_obj(layout);

    create_sensor_name_label(layout, remote_sensor);

    create_field_labels(layout, remote_sensor);

    create_chart(layout, remote_sensor);

    xSemaphoreGiveIfNotNull(xGuiSemaphore);
    return layout;
}

static void set_field_value(lv_obj_t *label, remote_sensor_field_t *field, double value)
{
    lv_label_set_text_fmt(label, "%s: %.1f%s", field->label, value, field->unit);
}

int layout_set_error(updater_layout_t *layout, remote_sensor_data_t *remote_sensor, lv_color_t color, SemaphoreHandle_t xGuiSemaphore)
{
    // if semaphore is provided, lock it
    xSemaphoreTakeIfNotNullOrFail(xGuiSemaphore, -1);

    lv_obj_set_style_outline_color(layout->parent, color, 0);
    for (int i = 0; i < remote_sensor->fields_count; i++)
    {
        set_field_value((layout->field_objects + i)->field, remote_sensor->fields + i, 0.0);
    }

    // unlock semaphore if it was given
    xSemaphoreGiveIfNotNull(xGuiSemaphore);
    return 0;
}

int layout_update_data(updater_layout_t *layout, remote_sensor_data_t *remote_sensor, SemaphoreHandle_t xGuiSemaphore)
{
    xSemaphoreTakeIfNotNullOrFail(xGuiSemaphore, -1);

    lv_chart_refresh(layout->chart);

    // if there are problems with sensor, set border to orange
    if (remote_sensor->error || !remote_sensor->device_online)
    {
        layout_set_error(layout, remote_sensor, lv_palette_main(LV_PALETTE_ORANGE), NULL);
        xSemaphoreGiveIfNotNull(xGuiSemaphore);
        return 0;
    }
    // set current values
    for (int i = 0; i < remote_sensor->fields_count; i++)
    {
        remote_sensor_field_t *field = remote_sensor->fields + i;
        set_field_value((layout->field_objects + i)->field, field, field->current_value);
    }
    lv_obj_set_style_outline_color(layout->parent, lv_palette_main(LV_PALETTE_GREY), 0);

    xSemaphoreGiveIfNotNull(xGuiSemaphore);

    return 0;
}

updater_layout_t *layout_reload(updater_layout_t *layout, remote_sensor_data_t *remote_sensor, SemaphoreHandle_t xGuiSemaphore)
{
    xSemaphoreTakeIfNotNullOrFail(xGuiSemaphore, NULL);

    layout_free(layout, NULL);
    updater_layout_t *new_layout = layout_init(remote_sensor, NULL);
    layout_update_data(new_layout, remote_sensor, NULL);

    xSemaphoreGiveIfNotNull(xGuiSemaphore);

    return new_layout;
}

void layout_free(updater_layout_t *layout, SemaphoreHandle_t xGuiSemaphore)
{
    if (!layout)
        return;

    xSemaphoreTakeIfNotNullOrFail(xGuiSemaphore, VOID);
    lv_obj_del(layout->parent);
    free(layout->field_objects);
    free(layout);
    xSemaphoreGiveIfNotNull(xGuiSemaphore);
}