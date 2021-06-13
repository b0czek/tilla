idf_component_register(SRCS "network_info.c" "network_tools.c" "connect.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_netif
                    )
