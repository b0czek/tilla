#include <string.h>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#if CONFIG_CONNECT_ETHERNET
#include "esp_eth.h"
#endif
#include "esp_log.h"
#include "esp_netif.h"

#include "network_tools.h"

bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

char *get_interface_short_name(esp_netif_t *netif)
{
    // getting description for interface, the string return from function will
    // look like "network_connect: ..." but you only want "..."
    const char *netif_desc = esp_netif_get_desc(netif);
    char *result = strstr(netif_desc, ":") + 2; //find colon substring and offset it to result
    return result;
}

#if CONFIG_WIFI_USE_STATIC_IP || CONFIG_ETHERNET_USE_STATIC_IP || CONFIG_USE_STATIC_DNS
ip4_union_t ip4_aton(const char *addr)
{
    ip4_union_t result = {
        .ip = esp_ip4addr_aton(addr),
    };
    return result;
}
#endif

#if CONFIG_WIFI_USE_STATIC_IP || CONFIG_ETHERNET_USE_STATIC_IP
esp_err_t set_static_ip(esp_netif_t *netif, esp_ip4_info_t *ip_config)
{
    esp_netif_dhcpc_stop(netif); // dhcp client is started automatically, so stop it
    esp_netif_ip_info_t ipconfig;
    for (int i = 0; i < 3; i++) // iterate through ip_config struct and set values in ipconfig
    {
        *(&ipconfig.ip + i) = ip4_aton((&ip_config->ip + i)[0]).ip4_addr;
    }
    ESP_LOGI(NETWORK_TAG, "Setting static ip for %s - ip: %s, netmask: %s, gateway: %s",
             get_interface_short_name(netif), ip_config->ip, ip_config->netmask, ip_config->gw);
    return esp_netif_set_ip_info(netif, &ipconfig);
}
#endif

#if CONFIG_USE_STATIC_DNS
esp_err_t set_static_dns(esp_netif_t *netif, esp_dns_info_t *dns_config)
{
    for (int i = 0; i < 2; i++)
    {
        esp_netif_dns_info_t config = dns_ip4_aton((&dns_config->main + i)[0]);
        esp_err_t result = esp_netif_set_dns_info(netif, i, &config);
        if (result != ESP_OK)
        {
            ESP_LOGI(NETWORK_TAG, "Something went wrong when setting static DNS for %s", get_interface_short_name(netif));
            return result;
        }
    }
    ESP_LOGI(NETWORK_TAG, "Setting static DNS for %s - %s and %s", get_interface_short_name(netif), dns_config->main, dns_config->backup);
    return ESP_OK;
}
#endif

esp_err_t create_mac_string(char *dest, size_t dest_len, const uint8_t *values, size_t val_len)
{
    if (dest_len < (val_len * 2 + 1)) /* check that dest is large enough */
        return ESP_ERR_INVALID_ARG;
    *dest = '\0'; /* in case val_len==0 */
    while (val_len--)
    {
        /* sprintf directly to where dest points */
        sprintf(dest, "%02X", *values);
        dest += 2;
        ++values;
        if (val_len) /* if its not the last byte, then add : separator */
        {
            sprintf(dest, "%c", ':');
            dest++;
        }
    }
    return ESP_OK;
}
esp_err_t create_ip_string(char *dest, size_t dest_len, esp_ip4_addr_t ip4_address)
{
    int buff_len = IP_STRING_LENGTH;
    char buff[buff_len];
    if (buff_len > dest_len)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ip4_union_t addr = {.ip4_addr = ip4_address};
    uint32_t ip = addr.ip;
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    snprintf(buff, buff_len, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    strncpy(dest, (char *)buff, buff_len);
    return ESP_OK;
}

#if CONFIG_USE_STATIC_DNS
esp_netif_dns_info_t dns_ip4_aton(const char *addr)
{
    esp_netif_dns_info_t result = {
        .ip = {
            .type = 0, // type is ipv4
            .u_addr = {
                .ip4 = ip4_aton(addr).ip4_addr,
            },
        },
    };
    return result;
}
#endif

esp_err_t dns_ip4_ntoa(char *dest, size_t dest_len, esp_netif_dns_info_t dns)
{
    if (dns.ip.type == 0) // if ip is ipv4, then proceed
    {
        return create_ip_string(dest, dest_len, dns.ip.u_addr.ip4);
    }
    else // if its ipv6, then set string to empty and return
    {
        dest[0] = '\0';
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}