#include "cJSON.h"
#include "esp_wifi.h"

#include "device.h"
#include "network.h"
#include "network_tools.h"
#include "network_info.h"

esp_network_info_t *get_network_info()
{
    esp_network_info_t *interfaces_info = malloc(NETWORK_INTERFACES_COUNT * sizeof(esp_network_info_t));
    if (get_esp_network_info(interfaces_info, NETWORK_INTERFACES_COUNT) != ESP_OK)
    {
        // if the function was unable to fetch interfaces information, then free memory and set pointer to null
        free(interfaces_info);
        interfaces_info = NULL;
    }
    return interfaces_info;
}

cJSON *get_network_info_json()
{
    esp_network_info_t *interfaces_info = get_network_info();
    cJSON *interfaces = cJSON_CreateObject();

    for (int i = 0; i < NETWORK_INTERFACES_COUNT; i++)
    {
        esp_network_info_t *netif = interfaces_info + i;
        cJSON *interface = cJSON_CreateObject();
        cJSON_AddStringToObject(interface, "mac_address", netif->mac);

        cJSON *ip_info = cJSON_CreateObject();
        cJSON_AddStringToObject(ip_info, "ip", netif->ip_info.ip);
        cJSON_AddStringToObject(ip_info, "netmask", netif->ip_info.netmask);
        cJSON_AddStringToObject(ip_info, "gw", netif->ip_info.gw);
        cJSON_AddItemToObject(interface, "ip_info", ip_info);

        cJSON *dns_info = cJSON_CreateObject();
        cJSON_AddStringToObject(dns_info, "primary_dns", netif->dns_info.main);
        cJSON_AddStringToObject(dns_info, "secondary_dns", netif->dns_info.backup);
        cJSON_AddItemToObject(interface, "dns", dns_info);
        cJSON_AddStringToObject(interface, "hostname", netif->hostname);

        cJSON_AddBoolToObject(interface, "connected", netif->connected);
        if (strcmp(netif->desc, "sta") == 0) // if netif is wifi client
        {
            cJSON *wifi_info = cJSON_CreateObject(); // create json object
            if (netif->wifi_info)                    // if pointer is not null, then add info about connection
            {
                int id_len = MAC_STRING_LENGH;
                char id[id_len];
                create_mac_string(id, id_len, netif->wifi_info->bssid, MAC_BYTES);
                cJSON_AddStringToObject(wifi_info, "bssid", id);

                cJSON_AddStringToObject(wifi_info, "ssid", (char *)netif->wifi_info->ssid);
                cJSON_AddNumberToObject(wifi_info, "rssi", netif->wifi_info->rssi);
                cJSON_AddNumberToObject(wifi_info, "auth_mode", (int)netif->wifi_info->authmode);
                cJSON_AddStringToObject(wifi_info, "country_code", netif->wifi_info->country.cc);
            }
            cJSON_AddItemToObject(interface, "wifi_info", wifi_info);
        }
        cJSON_AddItemToObject(interfaces, netif->desc, interface);
    }
    // cjson actually duplicates all the data so freeing original data's memory is safe
    free_esp_network_info(interfaces_info, NETWORK_INTERFACES_COUNT);
    return interfaces;
}
