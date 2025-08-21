#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <ttws/TTWS.h>
#include "internal/router.h"
#include "internal/server.h"

static int client_sockets[MAX_SOCKETS];
static int server_socket = -1;

struct TTWS_Server { // TODO: Add list of static routes/regex 
    int epoll_instance_fd;
    int socket_fd;
    int port_no;
    struct epoll_event events[MAX_SOCKETS];
    RouteNode route_trie_root;
    char** static_routes;
};

void cleanup_sockets() {
    close(server_socket);
    for(int i = 0; i < MAX_SOCKETS; i++) {
        if (client_sockets[i] >= 0) {
            //printf("KILLING: %d\n", client_sockets[i]);
            close(client_sockets[i]);
        }
    }
}

static struct epoll_event create_epoll_event() {
    struct epoll_event ev;
    ev.events = EPOLLIN;

    return ev;
}

TTWS_Server* TTWS_CreateServer(int port) {
    TTWS_Server* server = malloc(sizeof(TTWS_Server));
    server->epoll_instance_fd = epoll_create1(0);

    if (server->epoll_instance_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    server->port_no = port;
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (server->socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    server->route_trie_root.value = "/";
    server->route_trie_root.next = NULL;
    server->route_trie_root.no_children = 0;
    server->route_trie_root.children = NULL;
    server->route_trie_root.children = NULL;

    struct epoll_event ep_event = create_epoll_event();
    ep_event.data.fd = server->socket_fd;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server->socket_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    };
    return server;
}

char* read_entire_file(const char* filename, const int null_terminate) { //size_t* out_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    size_t capacity = 1024; // Initial buffer size
    size_t length = 0;
    char* buffer = malloc(capacity);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t n;
    while ((n = fread(buffer + length, 1, capacity - length, file)) > 0) {
        length += n;
        if (length == capacity) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                fclose(file);
                return NULL;
            }
            buffer = new_buffer;
        }
    }

    if (ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    //if (out_size) *out_size = length;
    char* null_terminated = realloc(buffer, length + 1);
    if (null_terminate && !null_terminated) {
        free(buffer);
        return NULL;
    }
    null_terminated[length] = '\0';
    return null_terminated; // Note: Not null-terminated
}

void handle_sigint(int sig) {
    printf("INTERUPT: %d\n", sig);
    cleanup_sockets();
    exit(0);
}

static int create_server_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    return socket_fd;
}


TTWS_Response* TTWS_CreateResponse() {
    TTWS_Response* res = malloc(sizeof(TTWS_Response));
    res->body = NULL;
    res->status = -1;

    return res;
}



static TTWS_Response* handle_request(const TTWS_Server* server, const char* req_str, TTWS_Response* response);

void TTWS_StartServer(TTWS_Server* server) {
    printf("\033[36mTeensyTinyWebServer\033[0m is listening on port \033[35m%d\033[0m.\n", server->port_no);
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
    
    if (listen(server->socket_fd, 5) == -1) {
        perror("listen");
        exit(1);
    }
    

    struct epoll_event ep_event = create_epoll_event();
    ep_event.data.fd = server->socket_fd;
    epoll_ctl( // epoll monitors sockets, this includes our listening socket. If I/O becomes available on that socket it means that another client is trying to connect and we should accept their connection, and add that clients new socket file descriptor ro
            server->epoll_instance_fd, 
            EPOLL_CTL_ADD, 
            server->socket_fd,
            &ep_event
            );

    // TODO:compile regex paths before main loop

    TTWS_Response* response = TTWS_CreateResponse();
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
                ep_event.events = EPOLLIN; // enable epollout if we still have pending writes
                ep_event.data.fd = client_fd;
                epoll_ctl(server->epoll_instance_fd, EPOLL_CTL_ADD, client_fd, &ep_event);
            } else {
                int bytes_read = recv(fd, buf, size - 1, 0);
                if (bytes_read <= 0) {
                    close(fd);
                    continue;
                }
                buf[bytes_read] = '\0';

                //printf("%s\n", buf);
                response = TTWS_CreateResponse();
                response = handle_request(server, buf, response);
                if (response != NULL) {
                    send(fd, response->body, strlen(response->body), 0);
                }
                //*(client_sockets+i) = client_fd;
                close(fd);
            }
        }
    }
}

static void parse_request_line(const char* request_line, TTWS_Request* request) {

    // Could potentially replace all of this with sscanf
    char* copy = strdup(request_line);
    char* method = strtok(copy, " ");
    if (method == NULL) {
        perror("malformed request");
        exit(1);
    }

    char* path = strtok(NULL, " ");
    if (path == NULL) {
        perror("malformed request");
        exit(1);
    }

    char* version = strtok(NULL, " ");
    if (version == NULL) {
        perror("malformed request");
        exit(1);
    }
    
    request->method = strdup(method);
    request->path = strdup(path);
    request->version = strdup(version);

    free(copy);
};

static TTWS_Response* handle_request(const TTWS_Server* server, const char* req_str, TTWS_Response* response) {
    TTWS_Request request;
    char* copy = strdup(req_str);
    char* line = strtok(copy, "\r\n");
    parse_request_line(line, &request);

    // TODO: Implement static serving here

    RouteHandler* route_handler = NULL;

    if (strcmp(request.path, "/") == 0) {
        route_handler = (RouteHandler*) &server->route_trie_root.handler;
    } else {
        route_handler = get_route_handler(server, &request);
    }

    if (route_handler && *route_handler) {
        (*route_handler)(&request, response);
    } else {
        printf("No handler found for path: %s\n", request.path);
        return NULL;
    }


    free(copy);
    free(request.method);
    free(request.path);
    free(request.version);

    return response;
}

#define HTTP_VERSION_1_1 "HTTP/1.1"

int TTWS_SendFile(TTWS_Response* res, const char* filepath, int status_code) {
    char* body = read_entire_file(filepath, 1);
    if (!body) {
        res->status = 404;
        res->body = strdup(
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found"
        );
        return -1;
    }

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, strlen(body));

    size_t total_len = strlen(header) + strlen(body) + 1;
    res->body = malloc(total_len);
    snprintf(res->body, total_len, "%s%s", header, body);
    res->status = status_code;

    free(body);
    return 0;
}
