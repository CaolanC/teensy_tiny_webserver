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
    int port_no;
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

    return ev;
}

TTWS_Server* TTWS_CreateServer(int port) {
    TTWS_Server* server = malloc(sizeof(TTWS_Server));
    server->epoll_instance_fd = epoll_create1(0);
    server->port_no = port;
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
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
// TODO: Clean up server->event = create_epoll_event(); we removed the event from the struct
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

int main() {
    TTWS_Server* server = TTWS_CreateServer(8080);
    TTWS_StartServer(server);
    return 0;
}
