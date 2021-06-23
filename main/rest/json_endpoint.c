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

json_node_t *json_endpoint_find(json_node_t *root_node, const char *path)
{
    remove_leading_slash(&path);
    printf("path %s\n", path);
    printf("root uri %s\n", root_node->uri_fragment);

    int node_uri_len = strlen(root_node->uri_fragment);
    if (strncmp(path, root_node->uri_fragment, node_uri_len) != 0)
    {
        return NULL;
    }
    path += node_uri_len;
    printf("searched in endpoint %s\n", path);
    if (*path == '\0')
    {
        return root_node;
    }

    for (int i = 0; i < root_node->child_nodes_count; i++)
    {
        json_node_t *searched_node = (root_node->child_nodes + i);
        json_node_t *found_node = json_endpoint_find(searched_node, path);
        if (found_node != NULL)
        {
            return found_node;
        }
    }
    return NULL;
}

void json_endpoint_add(json_node_t *root_node, const char *endpoint_path, json_node_endpoint_t *endpoint)
{
    remove_leading_slash(&endpoint_path);
    printf("endpoint before verification: %s \n", endpoint_path);
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
    printf("endpoint after verification: %s\n", endpoint_path);
}

json_node_t *json_endpoint_init(httpd_handle_t server, const char *base_path)
{
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

    return root_node;
}