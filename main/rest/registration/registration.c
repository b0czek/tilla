#include "registration.h"
#include "nvs_flash.h"

#define REJECT(req, message)                                            \
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, message); \
    return NULL;

cJSON *registration_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "is_registered", rest_register_check());
    cJSON_AddNumberToObject(root, "auth_key_len", AUTH_KEY_LENGTH);
    return root;
}
json_handler_auth_if_registered(handle_registration_info, registration_info);

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
    if (!cJSON_HasObjectItem(body_root, "auth_key"))
    {
        REJECT(req, "Missing authentication key");
    }
    cJSON *auth_key = cJSON_GetObjectItem(body_root, "auth_key");
    if (cJSON_IsInvalid(body_root) || strlen(auth_key->valuestring) != AUTH_KEY_LENGTH)
    {
        REJECT(req, "Invalid authentication key");
    }

    cJSON *root = cJSON_CreateObject();
    nvs_handle_t handle;
    nvs_open("rest", NVS_READWRITE, &handle);

    esp_err_t result = nvs_set_str(handle, "auth_key", auth_key->valuestring);
    nvs_commit(handle);
    nvs_close(handle);

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
    nvs_handle_t handle;

    nvs_open("rest", NVS_READWRITE, &handle);
    esp_err_t result = nvs_erase_key(handle, "auth_key");
    nvs_commit(handle);
    nvs_close(handle);
    cJSON *root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "error", result != 0);
    cJSON_AddNumberToObject(root, "code", result);
    return root;
}
json_handler_auth(handle_unregistration, unregister_device);

esp_err_t register_registration_handlers(httpd_handle_t *server)
{
    httpd_uri_t registration_info_uri = {
        .uri = "/api/v1/registration/info/?",
        .method = HTTP_GET,
        .handler = handle_registration_info};
    httpd_register_uri_handler(*server, &registration_info_uri);

    char *post_buffer = malloc(MAX_POST_CONTENT_LENGTH);
    httpd_uri_t registration_uri = {
        .uri = "/api/v1/registration/register/?",
        .method = HTTP_POST,
        .handler = handle_registration,
        .user_ctx = post_buffer};
    httpd_register_uri_handler(*server, &registration_uri);

    httpd_uri_t unregistration_uri = {
        .uri = "/api/v1/registration/unregister/?",
        .method = HTTP_GET,
        .handler = handle_unregistration};
    httpd_register_uri_handler(*server, &unregistration_uri);

    return ESP_OK;
}