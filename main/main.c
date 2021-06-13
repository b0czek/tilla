#include "sdkconfig.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"

#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

#include "network.h"
#include "rest_server.h"

#define MDNS_INSTANCE "esp home web server"

void start_reading_sensors();
static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}};

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());                // initialize tcp/ip stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // create an event loop
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_MDNS_HOST_NAME);

    ESP_ERROR_CHECK(network_connect());
    ESP_ERROR_CHECK(start_rest_server(CONFIG_WEB_MOUNT_POINT));
    start_reading_sensors();
}