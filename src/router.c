#include "internal/router.h"
#include "internal/request.h"
#include "internal/response.h"
#include "internal/server.h"
#include <ttws/TTWS.h>

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
typedef int (*RouteHandler)(const TTWS_Request* request, TTWS_Response* response);

struct TTWS_Server {
    int epoll_instance_fd;
    int socket_fd;
    int port_no;
    struct epoll_event events[MAX_SOCKETS];
    RouteNode route_trie_root;
    char** static_routes;
};

static RouteNode* create_route_node() {
    RouteNode* node = malloc(sizeof(RouteNode));
    node->no_children = 0;
    node->handler = NULL;
    node->value = NULL;
    node->children = NULL;

    return node;
}

static void add_route_to_children(RouteNode* parent, RouteNode* new_node) {
    parent->no_children++;
    RouteNode** result = realloc(parent->children, parent->no_children * sizeof(RouteNode*));

    if (result == NULL) {
        perror("realloc");
        exit(1);
    }
    parent->children = result;
    parent->children[parent->no_children-1] = new_node;
}

static void add_route_to_tier(RouteNode* node, char* route, char* remaining_path, RouteHandler* handler) {
    RouteNode* child_node;
    for(int i = 0; i < node->no_children; i++) {
        child_node = node->children[i];
        if (strcmp(route, child_node->value) == 0) {
            route = strtok_r(NULL, "/", &remaining_path);
            add_route_to_tier(child_node, route, remaining_path, handler);

            return;
        }
    }

    RouteNode* new_node = create_route_node();
    new_node->value = strdup(route);

    add_route_to_children(node, new_node);

    route = strtok_r(NULL, "/", &remaining_path);
    if (route == NULL) {
        new_node->handler = *handler;
        return;
    }

    add_route_to_tier(new_node, route, remaining_path, handler);
}

static void recurse_routes(RouteNode* node, char* path, char* save, int first_call) {
    if (node->value == NULL) {
        RouteNode* next_node = malloc(sizeof(RouteNode));
        next_node->value = NULL;
        next_node->next = NULL;
        node->next = next_node;
    }

    char* route;
    if (first_call) {
        route = strtok_r(path, "/", &save);
    } else {
        route = strtok_r(NULL, "/", &save);
    }
    if (route == NULL) {
        return;
    }
    printf("%s\n", route);
    recurse_routes(node, path, save, 0);
}

#define TREE_INDENT 2
static void print_trie_tree(const RouteNode* parent, int indent_level) {
    int offset = indent_level * TREE_INDENT;
    char indent[offset + 1];
    memset(indent, ' ', offset);
    indent[offset] = '\0';
    printf("%s%s\n", indent, parent->value ? parent->value : "(null)");
    for(int i = 0; i < parent->no_children; i++) {
        print_trie_tree(parent->children[i], indent_level + 1);
    }
}

void TTWS_PrintRouteTree(const TTWS_Server* server) {
    print_trie_tree(&server->route_trie_root, 0);
    printf("\n");
}

void TTWS_AddRoute(TTWS_Server* server, const char* method, const char* path, RouteHandler handler) {

    if (strcmp(path, "/") == 0) {
        server->route_trie_root.handler = handler;
        return;
    }

    char* path_copy = malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(path_copy, path);
    char* save;
    char* route = strtok_r(path_copy, "/", &save);

    add_route_to_tier(&server->route_trie_root, route, save, &handler);
    //recurse_routes(&server->route_trie_root, path_copy, NULL, 1);
}

static void get_route(TTWS_Server* server, char* path, char* save) {
    strtok_r(NULL, "/", &save);
}

RouteHandler* get_route_handler(const TTWS_Server* server, const TTWS_Request* request) {
    char* path = strdup(request->path);
    char* saveptr;
    char* segment = strtok_r(path, "/", &saveptr);

    RouteNode* current = (RouteNode*)&server->route_trie_root;

    while (segment != NULL) {
        int found = 0;

        for (int i = 0; i < current->no_children; i++) {
            if (strcmp(segment, current->children[i]->value) == 0) {
                current = current->children[i];
                found = 1;
                break;
            }
        }

        if (!found) {
            free(path);
            return NULL;
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    free(path);
    return current->handler ? &current->handler : NULL;
}
