#include "rest_utils.h"
#include "nvs_flash.h"
#include <esp_err.h>
#include <esp_http_server.h>
#include <cJSON.h>

#include "registration.h"

#define REST_AUTH_TAG "rest_auth"

bool rest_register_check()
{
    size_t length;
    nvs_handle_t handle;
    nvs_open("rest", NVS_READONLY, &handle);
    bool res = nvs_get_str(handle, "auth_key", 0, &length) == ESP_OK && length == AUTH_KEY_LENGTH + 1;
    nvs_close(handle);
    return res;
}

bool rest_auth_check(httpd_req_t *req)
{
    if (!rest_register_check())
    {
        ESP_LOGW(REST_AUTH_TAG, "device not registered");
        return false;
    }

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len < 1)
    {
        ESP_LOGW(REST_AUTH_TAG, "invalid req");
        return false;
    }
    char *buffer = malloc(sizeof(char) * buf_len);
    if (httpd_req_get_url_query_str(req, buffer, buf_len) != ESP_OK)
    {
        ESP_LOGW(REST_AUTH_TAG, "could not read url query");
        return false;
    }
    char key_buffer[AUTH_KEY_LENGTH + 1];
    if (httpd_query_key_value(buffer, "auth_key", key_buffer, sizeof(key_buffer)) != ESP_OK)
    {
        ESP_LOGW(REST_AUTH_TAG, "could not read key from req");
        return false;
    }
    key_buffer[AUTH_KEY_LENGTH] = '\0';
    free(buffer);

    size_t length;
    nvs_handle_t handle;
    nvs_open("rest", NVS_READONLY, &handle);

    if (nvs_get_str(handle, "auth_key", 0, &length) != ESP_OK)
    {
        ESP_LOGW(REST_AUTH_TAG, "could not read key length");
        return false;
    }

    char key[length];

    esp_err_t nvs_read = nvs_get_str(handle, "auth_key", key, &length);
    if (nvs_read != ESP_OK)
    {
        ESP_LOGW(REST_AUTH_TAG, "could not read key from nvs - %d", nvs_read);
        return false;
    }

    nvs_close(handle);

    return strcmp(key, key_buffer) == 0;
}

cJSON *unauthenticated_handler(httpd_req_t *req)
{

    httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    return NULL;
}

esp_err_t common_handler(httpd_req_t *req, cJSON *(fn)(httpd_req_t *))
{
    httpd_resp_set_type(req, "application/json");
    cJSON *response = fn(req);
    if (response)
    {
        return respond_json(req, response);
    }
    return ESP_OK;
}

esp_err_t respond_json(httpd_req_t *req, cJSON *res)
{
    const char *response_string = cJSON_PrintUnformatted(res);
    esp_err_t result = httpd_resp_sendstr(req, response_string);
    free((void *)response_string);
    cJSON_Delete(res);

    return result;
}