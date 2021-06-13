#include "network.h"

esp_err_t get_esp_network_info(esp_network_info_t *dest, size_t dest_len);

/**
 * @brief function used for deallocating memory from network_info structs
 * @param network_info pointer to struct
 */
void free_esp_network_info(esp_network_info_t *network_info, int count);