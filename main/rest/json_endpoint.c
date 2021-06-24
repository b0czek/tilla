#include <stdlib.h>

#include "json_endpoint.h"

// omit slash if it starts the string
static void remove_leading_slash(const char **string)
{
    if (**string == '/')
    {
        *string += 1;
    }
}

static json_node_t *root = NULL;

static esp_err_t json_httpd_handler(httpd_req_t *req)
{
    json_node_t *found_node = json_endpoint_find(root, req->uri, req->method);

    if (found_node == NULL)
    {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");

    cJSON *response = (*found_node->endpoint->handler)(req);
    const char *response_string = cJSON_PrintUnformatted(response);
    httpd_resp_sendstr(req, response_string);

    free((void *)response_string);
    cJSON_Delete(response);
    return ESP_OK;
}

/**
 * @returns pointer to appended child node in root node
 * 
*/
static json_node_t *json_child_append(json_node_t *root_node, const json_node_t *child_node)
{
    json_node_t *appended_node = NULL;
    // if there isn't any children at root node, then allocate memory for them
    if (root_node->child_nodes_count == 0)
    {
        root_node->child_nodes = malloc(sizeof(json_node_t));
        memcpy(root_node->child_nodes, child_node, sizeof(json_node_t));
        appended_node = root_node->child_nodes;
    }
    else
    {
        int children_count = root_node->child_nodes_count;
        root_node->child_nodes = realloc(root_node->child_nodes, sizeof(json_node_t) * children_count + 1);
        memcpy((root_node->child_nodes + children_count), child_node, sizeof(json_node_t));
        appended_node = (root_node->child_nodes + children_count);
    }
    // copy uri fragment
    appended_node->uri_fragment = malloc(strlen(child_node->uri_fragment) + 1);
    strcpy(appended_node->uri_fragment, child_node->uri_fragment);

    // if child is an endpoint, copy its properties
    if (child_node->endpoint != NULL)
    {
        appended_node->endpoint = malloc(sizeof(json_node_endpoint_t));
        memcpy(appended_node->endpoint, child_node->endpoint, sizeof(json_node_endpoint_t));
    }

    root_node->child_nodes_count++;
    return appended_node;
}

json_node_t *json_endpoint_find(json_node_t *root_node, const char *path, httpd_method_t method)
{
    remove_leading_slash(&path);

    int node_uri_len = strlen(root_node->uri_fragment);
    if (strncmp(path, root_node->uri_fragment, node_uri_len) != 0)
    {
        return NULL;
    }
    path += node_uri_len;

    // check if found node is an endpoint and if the method matches
    if (*path == '\0' &&
        root_node->endpoint != NULL &&
        root_node->endpoint->method == method)
    {
        return root_node;
    }

    for (int i = 0; i < root_node->child_nodes_count; i++)
    {
        json_node_t *searched_node = (root_node->child_nodes + i);
        json_node_t *found_node = json_endpoint_find(searched_node, path, method);
        if (found_node != NULL)
        {
            return found_node;
        }
    }
    return NULL;
}

bool json_endpoint_delete(json_node_t *deleted_node)
{
    // dont delete the anything if its root node
    if (deleted_node->parent_node == NULL)
    {
        return false;
    }
    // if its intermediate node, but it has its own endpoint, delete the endpoint
    if (deleted_node->child_nodes_count > 0 && deleted_node->endpoint != NULL)
    {
        free(deleted_node->endpoint);
        deleted_node->endpoint = NULL;
        return true;
    }
    // if its intermediate node, dont delete it
    else if (deleted_node->endpoint == NULL)
    {
        return false;
    }

    // if the node is at the end of the tree, delete it

    // destroy the node's members
    free(deleted_node->uri_fragment);
    free(deleted_node->endpoint);

    int parents_children_count = deleted_node->parent_node->child_nodes_count;
    // search for deleted node reference in parents children array and remove it
    for (int i = 0; i < parents_children_count; i++)
    {
        json_node_t *iterated_node = (deleted_node->parent_node->child_nodes + i);
        // if iterated node is the deleted one
        if (iterated_node == deleted_node)
        {
            // if child node is not the last one, shift all other members
            if ((i + 1) != parents_children_count)
            {
                int children_to_move = parents_children_count - (i + 1);
                memmove(iterated_node, (iterated_node + 1), sizeof(json_node_t) * children_to_move);
            }
            // realloc and free removes the node to be deleted
            // if parent has more than 1 child, reallocate their memory
            if (parents_children_count > 1)
            {
                // reallocate memory with 1 children less
                realloc(deleted_node->parent_node->child_nodes, sizeof(json_node_t) * parents_children_count);
            }
            // if parent has 1 child, free whole memory
            else
            {
                free(deleted_node->parent_node->child_nodes);
                deleted_node->parent_node->child_nodes = NULL;
            }

            // decrement children count
            deleted_node->parent_node->child_nodes_count--;
            break;
        }
    }

    // if deleted node is the only child and the parent node is not an endpoint itself
    // then delete parent node as well
    if (deleted_node->parent_node->child_nodes_count == 1 &&
        deleted_node->parent_node->endpoint == NULL)
    {
        json_endpoint_delete(deleted_node->parent_node);
    }
    return true;
}

json_node_t *json_endpoint_add(json_node_t *root_node, const char *endpoint_path, json_node_endpoint_t *endpoint)
{
    remove_leading_slash(&endpoint_path);

    // check if endpoint path starts with root's uri
    if (strncmp(endpoint_path, root_node->uri_fragment, strlen(root_node->uri_fragment)) == 0)
    {
        // offset path pointer
        endpoint_path += strlen(root_node->uri_fragment);
    }

    remove_leading_slash(&endpoint_path);

    // copy endpoint_path to mutable variable
    char *path = malloc(strlen(endpoint_path) + 1);
    strcpy(path, endpoint_path);

    char *fragment = strtok(path, "/");

    json_node_t new_node_config = {
        // path is trimmed in process of strtok
        .uri_fragment = path,
        // if there is not path to split anymore, add the endpoint config
        .endpoint = fragment == NULL ? endpoint : NULL,
        .parent_node = root_node,
        .child_nodes = NULL,
        .child_nodes_count = 0};

    json_node_t *appended_node = json_child_append(root_node, &new_node_config);

    if (fragment != NULL)
    {
        json_endpoint_add(appended_node, endpoint_path, endpoint);
    }

    free(path);
    return appended_node;
}

json_node_t *json_endpoint_init(httpd_handle_t server, const char *base_path)
{

    // register handlers in http server
    httpd_uri_t json_endpoint_uri_get = {
        .uri = base_path,
        .method = HTTP_GET,
        .handler = json_httpd_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &json_endpoint_uri_get);
    httpd_uri_t json_endpoint_uri_post = {
        .uri = base_path,
        .method = HTTP_POST,
        .handler = json_httpd_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &json_endpoint_uri_post);

    remove_leading_slash(&base_path);

    json_node_t *root_node = malloc(sizeof(json_node_t));

    int base_path_length = strlen(base_path);
    // if base_path has *,? or / on the end, then ignore it
    switch (base_path[base_path_length - 1])
    {
    case '*':
        // if there was question mark before asterisk, ignore it as well
        if (base_path[base_path_length - 2] == '?')
            base_path_length -= 1;
        __attribute__((fallthrough));
    case '?':
    case '/':
        base_path_length -= 1;
        break;
    }

    root_node->uri_fragment = malloc(base_path_length + 1); //acount for \0
    strncpy(root_node->uri_fragment, base_path, base_path_length);
    root_node->uri_fragment[base_path_length] = '\0'; // add \0 explicitly
    root_node->endpoint = NULL;
    root_node->parent_node = NULL;
    root_node->child_nodes = NULL;
    root_node->child_nodes_count = 0;
    root = root_node;
    return root_node;
}