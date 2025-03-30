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

struct TTWS_Server {
    int epoll_instance_fd;
    int socket_fd;
    struct epoll_event event;
    struct epoll_event events[MAX_SOCKETS];
    
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
    ev.data.fd = create_server_socket();

    return ev;
}

TTWS_Server* TTWS_CreateServer() {
    TTWS_Server* server = malloc(sizeof(TTWS_Server));
    server->epoll_instance_fd = epoll_create1(0);

    server->event = create_epoll_event();
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    epoll_ctl(
            server->epoll_instance_fd, 
            EPOLL_CTL_ADD, 
            server->socket_fd,
            &(server->event)
            );
    return server;
}

void TTWS_StartServer(TTWS_Server* server) {
    int no_ready_clients, client_fd;
    int size = 1024;
    char buf[size];
    const char* message = "HTTP/1.1 200 OK\n\r\n\r<h1>Hello World</h1>";

    for(;;) {
        no_ready_clients = epoll_wait(
            server->epoll_instance_fd,
            server->events,
            MAX_SOCKETS,
            -1  
            );

        for(int i = 0; i < no_ready_clients; i++) {
            client_fd = accept(server_socket, 0, 0);
            recv(client_fd, buf, size, 0);
            send(client_fd, message, strlen(message), 0);
            //*(client_sockets+i) = client_fd;
            close(client_fd);
        }
    }
}

void TTWS_AddStaticFile() {
    
}

int main() {
    TTWS_Server* server = TTWS_CreateServer();
    return 0;
}

/*
int main() {
    signal(SIGINT, handle_sigint);
    memset(client_sockets, -1, sizeof(client_sockets));
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(1);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*) &addr, sizeof(addr));
    listen(server_socket, 5);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_SOCKETS];
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;

    //memset(buf, 0, size);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev);
    int no_ready_fds, client_fd;

    for(int i = 0; i < 5; i++) {
        no_ready_fds = epoll_wait(epoll_fd, events, MAX_SOCKETS, -1);
    }
    cleanup_sockets();
    close(server_socket);
    return 0;
} */
