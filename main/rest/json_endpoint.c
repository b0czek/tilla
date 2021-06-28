#include <stdlib.h>
#include <cJSON.h>
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

static cJSON *generate_tree(json_node_t *node)
{
    cJSON *nodes = cJSON_CreateObject();
    for (int i = 0; i < node->child_nodes_count; i++)
    {
        json_node_t *iterated_node = (node->child_nodes + i);
        cJSON *iterated_tree = generate_tree(iterated_node);
        // if node is not an endpoint, pointer will stay pointing to path shard
        char *tree_name = iterated_node->uri_fragment;
        // if the node is an endpoint and not just an intermediate node, append @ and method at the end of its name
        if (iterated_node->endpoint != NULL)
        {
            const char *method = NULL;
            switch (iterated_node->endpoint->method)
            {
            case HTTP_GET:
                method = &"GET";
                break;
            case HTTP_POST:
                method = &"POST";
                break;
            default:
                method = &"UNKNOWN";
                break;
            }

            int uri_fragment_len = strlen(iterated_node->uri_fragment);
            // path shard + @ + method + \0
            tree_name = malloc(uri_fragment_len + 2 + strlen(method));
            sprintf(tree_name, "%s@%s", iterated_node->uri_fragment, method);
        }

        cJSON_AddItemToObject(nodes, tree_name, iterated_tree);
        if (iterated_node->endpoint != NULL)
        {
            free(tree_name);
        }
    }
    return nodes;
}

cJSON *get_routing_tree_json(httpd_req_t *req)
{
    // create a fake father node to the root node and kickstart the recursive function
    json_node_t *grandfather_node = calloc(1, sizeof(json_node_t));
    grandfather_node->child_nodes = root;
    grandfather_node->child_nodes_count = 1;
    cJSON *tree = generate_tree(grandfather_node);
    free(grandfather_node);
    return tree;
}

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

static json_node_t *find_child_by_uri(json_node_t *parent_node, const char *searched_uri)
{
    for (int i = 0; i < parent_node->child_nodes_count; i++)
    {
        json_node_t *iterated_child = (parent_node->child_nodes + i);
        if (strcmp(iterated_child->uri_fragment, searched_uri) == 0)
        {
            return iterated_child;
        }
    }
    return NULL;
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
        root_node->child_nodes = realloc(root_node->child_nodes, sizeof(json_node_t) * (children_count + 1));
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
    printf("searching for path %s in node %s\n", path, root_node->uri_fragment);

    int node_uri_len = strlen(root_node->uri_fragment);
    // if searched node does not start with the same prefix
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
    printf("proceeding with deletion of node %s\n", deleted_node->uri_fragment);
    // dont delete anything if its root node
    if (deleted_node->parent_node == NULL)
    {
        printf("not deleting %s cause its root node\n", deleted_node->uri_fragment);
        return false;
    }
    // if its intermediate node, but it has its own endpoint, delete the endpoint
    if (deleted_node->child_nodes_count > 0 && deleted_node->endpoint != NULL)
    {
        printf("deleting only endpoint since node %s has its children \n", deleted_node->uri_fragment);
        free(deleted_node->endpoint);
        deleted_node->endpoint = NULL;
        return true;
    }
    // if its intermediate node, but it hasnt got its own endpoint, then dont delete the node
    else if (deleted_node->child_nodes_count > 0)
    {
        printf("not deleting node %s, because its got children \n", deleted_node->uri_fragment);
        return false;
    }

    // if the node is at the end of the branch, delete it

    // destroy the node's members
    printf("deleting node %s\n", deleted_node->uri_fragment);
    // free(deleted_node->uri_fragment);
    // free(deleted_node->endpoint);

    int parents_children_count = deleted_node->parent_node->child_nodes_count;
    // search for deleted node reference in parents children array and remove it
    for (int i = 0; i < parents_children_count; i++)
    {
        json_node_t *iterated_node = (deleted_node->parent_node->child_nodes + i);
        if (iterated_node != deleted_node)
        {
            continue;
        }
        // if iterated node is the deleted one

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
            realloc(deleted_node->parent_node->child_nodes, sizeof(json_node_t) * (parents_children_count - 1));
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

    // try to delete parent if it isnt an endpoint
    if (deleted_node->parent_node->endpoint == NULL)
    {
        json_endpoint_delete(deleted_node->parent_node);
    }
    return true;
}

json_node_t *json_endpoint_add(json_node_t *root_node, const char *endpoint_path, json_node_endpoint_t *endpoint)
{
    // printf("adding node %s to node %s\n", endpoint_path, root_node->uri_fragment);

    remove_leading_slash(&endpoint_path);

    // check if node uri starts with root's uri
    if (strncmp(endpoint_path, root_node->uri_fragment, strlen(root_node->uri_fragment)) == 0)
    {
        // if so, offset path pointer
        endpoint_path += strlen(root_node->uri_fragment);
    }

    remove_leading_slash(&endpoint_path);

    // search for node delimiter in uri
    char *fragment = strstr(endpoint_path, "/");

    // if there is a slash, copy the trimmed fragment, if not, copy whole stirng
    int uri_length = fragment == NULL ? strlen(endpoint_path) + 1 : fragment - endpoint_path + 1;
    char *uri_fragment = malloc(uri_length);
    strlcpy(uri_fragment, endpoint_path, uri_length);

    // search for node with the same uri fragment in root
    json_node_t *operating_node = find_child_by_uri(root_node, uri_fragment);

    // if there is no node with the same uri, append new node
    if (operating_node == NULL)
    {
        printf("no node %s found in %s, appending new node %s \n", endpoint_path, root_node->uri_fragment, uri_fragment);
        json_node_t new_node_config = {
            // path is trimmed in process of substringing
            .uri_fragment = uri_fragment,
            // if there is not path to split anymore, add the endpoint config
            .endpoint = fragment == NULL ? endpoint : NULL,
            .parent_node = root_node,
            .child_nodes = NULL,
            .child_nodes_count = 0};

        operating_node = json_child_append(root_node, &new_node_config);
        free(uri_fragment);
    }
    // if there is another section('/') in requsted path
    if (fragment != NULL)
    {
        operating_node = json_endpoint_add(operating_node, endpoint_path, endpoint);
    }
    return operating_node;
}

static char *trim_uri(const char *uri)
{

    int uri_len = strlen(uri);
    const char *last_character = uri + uri_len - 1;
    // if base_path has *,? or / on the end, then ignore it
    while (*last_character == '/' ||
           *last_character == '?' ||
           *last_character == '*')
    {
        uri_len--;
        last_character = uri + uri_len - 1;
    }

    char *trimmed_uri = malloc(uri_len + 1); //acount for \0
    strncpy(trimmed_uri, uri, uri_len);
    trimmed_uri[uri_len] = '\0'; // add \0 explicitly
    return trimmed_uri;
}

static esp_err_t register_uri_handler(httpd_handle_t server, const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
    // trim the string
    const char *trimmed_uri = trim_uri(uri);

    int uri_len = strlen(trimmed_uri);
    char *handler_uri = malloc(uri_len + 4);
    // copy it into new memory
    memcpy(handler_uri, trimmed_uri, uri_len);
    // add /?* at the end of uri
    memcpy(handler_uri + uri_len, &"/?*", 4);
    // release trimmed string
    free((void *)trimmed_uri);

    // create httpd uri
    httpd_uri_t uri_config = {
        .uri = handler_uri,
        .method = method,
        .handler = handler,
        .user_ctx = NULL};
    // register handler
    esp_err_t result = httpd_register_uri_handler(server, &uri_config);
    free(handler_uri);
    return result;
}

json_node_t *json_endpoint_init(httpd_handle_t server, const char *base_path)
{
    // register handlers in http server
    register_uri_handler(server, base_path, HTTP_GET, json_httpd_handler);
    register_uri_handler(server, base_path, HTTP_POST, json_httpd_handler);

    json_node_t *root_node = malloc(sizeof(json_node_t));

    remove_leading_slash(&base_path);
    char *uri = trim_uri(base_path);
    root_node->uri_fragment = uri;

    root_node->endpoint = NULL;
    root_node->parent_node = NULL;
    root_node->child_nodes = NULL;
    root_node->child_nodes_count = 0;
    root = root_node;
    return root_node;
}