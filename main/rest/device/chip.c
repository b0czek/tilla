#include "esp_system.h"

#include "device.h"
#include "network.h"
#include "network_tools.h"

cJSON *get_chip_info_json()
{
    cJSON *chip = cJSON_CreateObject();
    esp_chip_info_t *chip_info = malloc(sizeof(esp_chip_info_t));
    esp_chip_info(chip_info);
    cJSON_AddNumberToObject(chip, "chip_model", (int)chip_info->model);
    cJSON_AddNumberToObject(chip, "features", chip_info->features);
    cJSON_AddNumberToObject(chip, "cores", chip_info->cores);
    cJSON_AddNumberToObject(chip, "revision", chip_info->revision);
    free(chip_info);

    // add chip_id(factory programmed mac address)
    uint8_t mac[MAC_BYTES];
    esp_efuse_mac_get_default(mac);
    char id[MAC_STRING_LENGH];
    create_mac_string(id, MAC_STRING_LENGH, mac, MAC_BYTES);
    cJSON_AddStringToObject(chip, "chip_id", id);

    return chip;
}

/* Handler for getting device's chip info */
esp_err_t chip_data_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");

    cJSON *root = get_chip_info_json();

    const char *response = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, response);

    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}
