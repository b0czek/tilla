#pragma once

#include <esp_http_server.h>
#include <cJSON.h>
#include "HttpStatusCodes_C.h"

typedef struct
{

    httpd_method_t method;

    enum HttpStatus_Code (*handler)(httpd_req_t *r, cJSON *target);

} json_node_endpoint_t;

typedef struct json_node_t
{
    char *uri_fragment;

    json_node_endpoint_t *endpoint;

    struct json_node_t *parent_node;

    struct json_node_t *child_nodes;
    uint32_t child_nodes_count;
} json_node_t;

json_node_t *json_endpoint_find(json_node_t *root_node, const char *path);
void json_endpoint_add(json_node_t *root_node, const char *endpoint_path, json_node_endpoint_t *endpoint);
json_node_t *json_endpoint_init(httpd_handle_t server, const char *base_path);