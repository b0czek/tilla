idf_component_register(SRCS "esp_network.c" "network_info.c" "network_tools.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_netif
                    )
