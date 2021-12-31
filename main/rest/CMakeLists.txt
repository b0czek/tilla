idf_component_register(
                    SRC_DIRS "." "device" "sensors" "registration"
                    INCLUDE_DIRS "." "device" "sensors" "registration"
                    PRIV_REQUIRES "esp_http_server json network vec drivers esp32-ds18b20 esp32-owb bme280 nvs_flash")

