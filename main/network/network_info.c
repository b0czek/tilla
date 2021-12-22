#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"
#include "network.h"

#include "network_tools.h"

esp_err_t get_esp_network_info(esp_network_info_t *dest, size_t dest_len)
{
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    esp_netif_dns_info_t dns;
    if (esp_netif_get_nr_of_ifs() > dest_len)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i)
    {
        netif = esp_netif_next(netif);
        if (!is_our_netif(NETWORK_TAG, netif)) // skip if interface is not "our" somehow
        {
            continue;
        }

        // fetching mac address
        uint8_t mac[6];
        esp_netif_get_mac(netif, mac);
        // formatting mac bytes into string
        create_mac_string(dest->mac, MAC_STRING_LENGH, mac, MAC_BYTES);

        const char *hostname = NULL;
        esp_netif_get_hostname(netif, &hostname);
        strcpy(dest->hostname, hostname);

        // fetching ip data
        ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));

        for (int i = 0; i < 3; i++) // iterate for ip,netmask,gw
        {
            char *ip_dest = (&dest->ip_info.ip + i)[0];
            create_ip_string(ip_dest, IP_STRING_LENGTH, *(&ip.ip + i));
        }

        //dns data
        // ESP_NETIF_DNS_MAIN is 0, ESP_NETIF_DNS_BACKUP is 1
        for (int i = 0; i < 2; i++)
        {
            char *dns_dest = (&dest->dns_info.main + i)[0];
            esp_netif_get_dns_info(netif, i, &dns);
            dns_ip4_ntoa(dns_dest, IP_STRING_LENGTH, dns);
        }

        strcpy(dest->desc, get_interface_short_name(netif));
        // assigning \0 at the end to be certain that string will terminate
        dest->desc[member_size(esp_network_info_t, desc) - 1] = '\0';

        dest->connected = esp_netif_is_netif_up(netif);

        // check if dhcpc is stopped, thus the address is static
        esp_netif_dhcp_status_t dhcp_status;
        esp_netif_dhcpc_get_status(netif, &dhcp_status);
        dest->is_static = dhcp_status == ESP_NETIF_DHCP_STOPPED;

        dest->wifi_info = NULL;
        if (strcmp(dest->desc, "sta") == 0) // if the interface is wifi client
        {
            wifi_ap_record_t *apinfo = malloc(sizeof(wifi_ap_record_t));
            // add the wifi_ap_record
            esp_err_t ap_info_result = esp_wifi_sta_get_ap_info(apinfo);
            if (ap_info_result == ESP_OK)
            {
                dest->wifi_info = apinfo; // pointer assignment, memory should be freed only in free_esp_network_info
            }
            else // if there was a fail, allocated memory won't be needed anymore
            {
                free(apinfo);
            }
        }
        dest++;
    }
    // freeing netif pointer would delete actual interface it is pointing to
    return ESP_OK;
}

void free_esp_network_info(esp_network_info_t *network_info, int count)
{
    for (int i = 0; i < count; i++)
    {
        free((network_info + i)->wifi_info);
    }
    free(network_info);
    network_info = NULL;
}