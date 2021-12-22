#pragma once

#include "network.h"
#include "sdkconfig.h"

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the module NETWORK_TAG,
 * so it returns true if the specified netif is owned by this module
 */
bool is_our_netif(const char *prefix, esp_netif_t *netif);

char *get_interface_short_name(esp_netif_t *netif);

#if CONFIG_WIFI_USE_STATIC_IP || CONFIG_ETHERNET_USE_STATIC_IP || CONFIG_USE_STATIC_DNS
ip4_union_t ip4_aton(const char *addr);
#endif

#if CONFIG_WIFI_USE_STATIC_IP || CONFIG_ETHERNET_USE_STATIC_IP
esp_err_t set_static_ip(esp_netif_t *netif, esp_ip4_info_t *ip_config);
#endif

#if CONFIG_USE_STATIC_DNS
esp_err_t set_static_dns(esp_netif_t *netif, esp_dns_info_t *dns_config);
#endif

esp_err_t create_mac_string(char *dest, size_t dest_len, const uint8_t *values, size_t val_len);

esp_err_t create_ip_string(char *dest, size_t dest_len, esp_ip4_addr_t ip4_address);

esp_err_t dns_ip4_ntoa(char *dest, size_t dest_len, esp_netif_dns_info_t dns);

#if CONFIG_USE_STATIC_DNS
esp_netif_dns_info_t dns_ip4_aton(const char *addr);
#endif