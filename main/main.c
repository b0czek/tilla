#include "sdkconfig.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "network.h"
#include "rest_server.h"
#include "drivers.h"

#ifdef CONFIG_USE_DISPLAY
#include "display.h"
#include "updater.h"
#endif

#define MDNS_INSTANCE "esp home web server"

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}};

    ESP_ERROR_CHECK(mdns_service_add(CONFIG_MDNS_HOST_NAME, "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(init_fs());
    ESP_ERROR_CHECK(esp_netif_init());                // initialize tcp/ip stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // create an event loop
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_MDNS_HOST_NAME);

    sensor_drivers_t *drivers = init_drivers();

#ifdef CONFIG_USE_DISPLAY
    SemaphoreHandle_t xGuiSemaphore = init_display(drivers);
#endif
    ESP_ERROR_CHECK(network_connect());
    ESP_ERROR_CHECK(start_rest_server(drivers));
    // init the updater after the network has connected
#ifdef CONFIG_USE_DISPLAY
    display_updater_init(xGuiSemaphore);
#endif
}
