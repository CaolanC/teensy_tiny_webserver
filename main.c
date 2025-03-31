#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "./TTWS.h"

#define MAX_SOCKETS 20

static int client_sockets[MAX_SOCKETS];
static int server_socket = -1;

void cleanup_sockets() {
    close(server_socket);
    for(int i = 0; i < MAX_SOCKETS; i++) {
        if (client_sockets[i] >= 0) {
            //printf("KILLING: %d\n", client_sockets[i]);
            close(client_sockets[i]);
        }
    }
}

void handle_sigint(int sig) {
    printf("INTERUPT: %d\n", sig);
    cleanup_sockets();
    exit(0);
}


typedef void (*RouteHandler)();
typedef struct RouteNode {
    char* value;
    struct RouteNode* next;
    struct RouteNode** children;
    int no_children;
    RouteHandler handler;
} RouteNode;

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

struct TTWS_Server {
    int epoll_instance_fd;
    int socket_fd;
    int port_no;
    struct epoll_event events[MAX_SOCKETS];
    RouteNode route_trie_root;
};

static int create_server_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    return socket_fd;
}

static struct epoll_event create_epoll_event() {
    struct epoll_event ev;
    ev.events = EPOLLIN;

    return ev;
}


TTWS_Server* TTWS_CreateServer(int port) {
    TTWS_Server* server = malloc(sizeof(TTWS_Server));
    server->epoll_instance_fd = epoll_create1(0);
    server->port_no = port;
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server->route_trie_root.value = "/";
    server->route_trie_root.next = NULL;
    struct epoll_event ep_event = create_epoll_event();
    ep_event.data.fd = server->socket_fd;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server->socket_fd, (struct sockaddr*) &addr, sizeof(addr));
    return server;
}

void TTWS_StartServer(TTWS_Server* server) {
    printf("\033[36mTinyWebServer\033[0m is listening on port \033[35m%d\033[0m.\n", server->port_no);
    int no_ready_clients, client_fd, fd;
    int size = 1024;
    char buf[size];

    const char* body = "<h1>Hello World</h1>";
    char message[1024];
    snprintf(message, sizeof(message),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);
    listen(server->socket_fd, 5);
    struct epoll_event ep_event = create_epoll_event();
    ep_event.data.fd = server->socket_fd;
    epoll_ctl( // epoll monitors sockets, this includes our listening socket. If I/O becomes available on that socket it means that another client is trying to connect and we should accept their connection, and add that clients new socket file descriptor ro
            server->epoll_instance_fd, 
            EPOLL_CTL_ADD, 
            server->socket_fd,
            &ep_event
            );
    for(;;) {
        no_ready_clients = epoll_wait(
            server->epoll_instance_fd,
            server->events,
            MAX_SOCKETS,
            -1  
            );

        for(int i = 0; i < no_ready_clients; i++) {
            fd = server->events[i].data.fd;
            if (fd == server->socket_fd) { // The socket that is available for I/O is our listen server, meaning that a new possible client is trying to connect.
                client_fd = accept(server->socket_fd, 0, 0);
                ep_event.events = (EPOLLIN | EPOLLOUT);
                ep_event.data.fd = client_fd;
                epoll_ctl(server->epoll_instance_fd, EPOLL_CTL_ADD, client_fd, &ep_event);
            } else {
                recv(fd, buf, size, 0);
                send(fd, message, strlen(message), 0);
                //*(client_sockets+i) = client_fd;
                close(fd);
            }
        }
    }
}

void TTWS_AddStaticFile() {
    
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

static void add_route_to_tier(RouteNode* node, char* route, char* remaining_path) {
    RouteNode* child_node;
    for(int i = 0; i < node->no_children; i++) {
        child_node = node->children[i];
        if (strcmp(route, child_node->value) == 0) {
            route = strtok_r(NULL, "/", &remaining_path);
            add_route_to_tier(child_node, route, remaining_path);

            return;
        }
    }

    RouteNode* new_node = create_route_node();
    new_node->value = route;

    add_route_to_children(node, new_node);

    route = strtok_r(NULL, "/", &remaining_path);
    if (route == NULL) {
        return;
    }

    add_route_to_tier(new_node, route, remaining_path);

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

    add_route_to_tier(&server->route_trie_root, route, save);
    //recurse_routes(&server->route_trie_root, path_copy, NULL, 1);
}

// Go through each of the roots children
// if there's a match, enter that route, if there isn't create a new one
//

void handle() {

}

// TODO: Add a --print-routes flag and print a tree structure.
int main() {
    TTWS_Server* server = TTWS_CreateServer(8080);
    TTWS_AddRoute(server, "GET", "/my_path/lol/", handle);
    TTWS_AddRoute(server, "GET", "/my_path/cheese/", handle);
    TTWS_AddRoute(server, "GET", "/my_path/cheese/gouda", handle);
    TTWS_AddRoute(server, "GET", "/my_path/cheese/fries", handle);
    TTWS_AddRoute(server, "GET", "/my_path/elmo", handle);
    TTWS_AddRoute(server, "GET", "/the_jiggler", handle);

    TTWS_PrintRouteTree(server);
    //TTWS_StartServer(server);
    return 0;
}
