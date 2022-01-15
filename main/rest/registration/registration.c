#include "registration.h"
#include "nvs_flash.h"
#include "nvs_utils.h"

#define REJECT(req, message)                                            \
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, message); \
    return NULL;

static cJSON *get_json_field(cJSON *object, const char *field_name)
{
    if (!cJSON_HasObjectItem(object, field_name))
    {
        return NULL;
    }
    cJSON *field = cJSON_GetObjectItem(object, field_name);

    if (cJSON_IsInvalid(field))
    {
        return NULL;
    }
    return field;
}

cJSON *registration_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    bool is_registered = rest_register_check();
    cJSON_AddBoolToObject(root, "is_registered", is_registered);

    if (is_registered)
    {
        char *device_uuid = read_nvs_str("device_uuid");
        cJSON_AddStringToObject(root, "device_uuid", device_uuid);
        free(device_uuid);
    }
    else
    {
        cJSON_AddNullToObject(root, "device_uuid");
    }

    cJSON_AddNumberToObject(root, "auth_key_len", AUTH_KEY_LENGTH);
    return root;
}
json_handler(handle_registration_info, registration_info);

cJSON *register_device(httpd_req_t *req)
{
    if (rest_register_check())
    {
        REJECT(req, "Device already registered");
    }
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = (char *)(req->user_ctx);
    int received = 0;
    if (total_len >= MAX_POST_CONTENT_LENGTH)
    {
        REJECT(req, "content too long");
    }

    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            REJECT(req, "Failed to post control value");
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *body_root = cJSON_Parse(buf);
    if (!body_root || cJSON_IsInvalid(body_root))
    {
        REJECT(req, "Failed to parse data");
    }

    cJSON *auth_key = get_json_field(body_root, "auth_key");
    if (auth_key == NULL || !cJSON_IsString(auth_key) || strlen(auth_key->valuestring) != AUTH_KEY_LENGTH)
    {
        REJECT(req, "Invalid authentication key");
    }

    cJSON *device_uuid = get_json_field(body_root, "device_uuid");
    if (device_uuid == NULL || !cJSON_IsString(device_uuid) || strlen(device_uuid->valuestring) != UUID_LENGTH)
    {
        REJECT(req, "Invalid device uuid");
    }

    cJSON *callback_host = get_json_field(body_root, "callback_host");
    if (callback_host == NULL || !cJSON_IsString(callback_host))
    {
        REJECT(req, "Invalid callback host");
    }

    cJSON *callback_port = get_json_field(body_root, "callback_port");
    if (callback_port == NULL || !cJSON_IsNumber(callback_port))
    {
        REJECT(req, "Invalid callback port");
    }

    esp_err_t result = 0;

    nvs_handle_t handle;
    nvs_open("rest", NVS_READWRITE, &handle);

    result += nvs_set_str(handle, "auth_key", auth_key->valuestring);
    result += nvs_set_str(handle, "device_uuid", device_uuid->valuestring);
    result += nvs_set_str(handle, "callback_host", callback_host->valuestring);
    result += nvs_set_i32(handle, "callback_port", callback_port->valueint);
    result += nvs_commit(handle);
    nvs_close(handle);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "error", result != 0);
    cJSON_AddNumberToObject(root, "code", result);
    return root;
}

json_handler(handle_registration, register_device);

cJSON *unregister_device(httpd_req_t *req)
{
    if (!rest_register_check())
    {
        REJECT(req, "Device already unregistered");
    }
    esp_err_t result = 0;
    nvs_handle_t handle;

    nvs_open("rest", NVS_READWRITE, &handle);
    result += nvs_erase_key(handle, "auth_key");
    result += nvs_erase_key(handle, "device_uuid");
    result += nvs_erase_key(handle, "callback_host");
    result += nvs_erase_key(handle, "callback_port");
    result += nvs_commit(handle);
    nvs_close(handle);
    cJSON *root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "error", result != 0);
    cJSON_AddNumberToObject(root, "code", result);
    return root;
}
json_handler_auth(handle_unregistration, unregister_device);

esp_err_t register_registration_handlers(httpd_handle_t server)
{
    esp_err_t result = 0;
    httpd_uri_t registration_info_uri = {
        .uri = "/api/v1/registration/info/?",
        .method = HTTP_GET,
        .handler = handle_registration_info};
    result += httpd_register_uri_handler(server, &registration_info_uri);

    char *post_buffer = malloc(MAX_POST_CONTENT_LENGTH);
    httpd_uri_t registration_uri = {
        .uri = "/api/v1/registration/register/?",
        .method = HTTP_POST,
        .handler = handle_registration,
        .user_ctx = post_buffer};
    result += httpd_register_uri_handler(server, &registration_uri);

    httpd_uri_t unregistration_uri = {
        .uri = "/api/v1/registration/unregister/?",
        .method = HTTP_GET,
        .handler = handle_unregistration};
    result += httpd_register_uri_handler(server, &unregistration_uri);

    return result;
}