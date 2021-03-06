/* Common functions for programs, to establish Wi-Fi or Ethernet connection.

   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_netif.h"

#ifdef CONFIG_CONNECT_ETHERNET
#define NETWORK_INTERFACE network_get_netif()
#endif

#ifdef CONFIG_CONNECT_WIFI
#define NETWORK_INTERFACE network_get_netif()
#endif

/**
 * @brief Configure Wi-Fi or Ethernet, connect, wait for IP
 *
 * This all-in-one helper function is used in protocols examples to
 * reduce the amount of boilerplate in the example.
 *
 * It is not intended to be used in real world applications.
 * See examples under examples/wifi/getting_started/ and examples/ethernet/
 * for more complete Wi-Fi or Ethernet initialization code.
 *
 * Read "Establishing Wi-Fi or Ethernet Connection" section in
 * examples/protocols/README.md for more information about this function.
 *
 * @return ESP_OK on successful connection
 */
esp_err_t network_connect(void);

/**
 * Counterpart to network_connect, de-initializes Wi-Fi or Ethernet
 */
esp_err_t network_disconnect(void);

/**
 * @brief Configure stdin and stdout to use blocking I/O
 *
 * This helper function is used in ASIO examples. It wraps installing the
 * UART driver and configuring VFS layer to use UART driver for console I/O.
 */
esp_err_t network_configure_stdin_stdout(void);

/**
 * @brief Returns esp-netif pointer created by network_connect()
 *
 * @note If multiple interfaces active at once, this API return NULL
 * In that case the network_get_netif_from_desc() should be used
 * to get esp-netif pointer based on interface description
 */
esp_netif_t *network_get_netif(void);

/**
 * @brief Returns esp-netif pointer created by network_connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 */
esp_netif_t *network_get_netif_from_desc(const char *desc);

#define member_size(type, member) sizeof(((type *)0)->member) // cast nullpointer to struct and get member size
#define MAC_BYTES 6
#define IP_STRING_LENGTH 16 
#define MAC_STRING_LENGH 6*2+6  // 6 bytes, 2 chars per byte, colons between them(5) and \0 at the end

typedef struct {
    char ip[IP_STRING_LENGTH];
    char netmask[IP_STRING_LENGTH];
    char gw[IP_STRING_LENGTH];
} esp_ip4_info_t;

typedef struct {
    char primary[IP_STRING_LENGTH];
    char secondary[IP_STRING_LENGTH];
} esp_dns_info_t;

typedef struct 
{
    char mac[6*2+6];
    char hostname[33]; // max hostname length is 32 characters
    char desc[4];
    esp_ip4_info_t ip_info;
    esp_dns_info_t dns_info;
    bool connected;

    wifi_ap_record_t *wifi_info;
} esp_network_info_t;

esp_err_t create_mac_string(char *dest, size_t dest_len, const uint8_t *values, size_t val_len);

esp_err_t get_esp_network_info(esp_network_info_t *dest, size_t dest_len);

/**
 * @brief function used for deallocating memory from network_info structs
 * @param network_info pointer to struct
 */
void free_esp_network_info(esp_network_info_t *network_info, int count);

#ifdef __cplusplus
}
#endif
