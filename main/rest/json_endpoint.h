#pragma once

#include <esp_http_server.h>
#include <cJSON.h>

typedef struct
{

    httpd_method_t method;

    cJSON *(*handler)(httpd_req_t *r);

} json_node_endpoint_t;

typedef struct json_node_t
{
    char *uri_fragment;

    json_node_endpoint_t *endpoint;

    struct json_node_t *parent_node;

    struct json_node_t *child_nodes;
    uint32_t child_nodes_count;
} json_node_t;

json_node_t *json_endpoint_find(json_node_t *root_node, const char *path, httpd_method_t method);
bool json_endpoint_delete(json_node_t *deleted_node);
json_node_t *json_endpoint_add(json_node_t *root_node, const char *endpoint_path, json_node_endpoint_t *endpoint);
json_node_t *json_endpoint_init(httpd_handle_t server, const char *base_path);