idf_component_register(
                    SRC_DIRS "." "fetcher" "updater" "peripherals"
                    INCLUDE_DIRS "include" "fetcher" "peripherals"
                    PRIV_REQUIRES lvgl lvgl_esp32_drivers drivers esp32-rotary-encoder esp32-owb esp32-ds18b20 vec bme280 rest nvs_flash esp_http_client json)



                    
