#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

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
    int size = 1024;
    char buf[size];
    memset(buf, 0, size);
    for(int i = 0; i < 5; i++) {
        int client_fd = accept(server_socket, 0, 0);
        recv(client_fd, buf, size, 0);
        printf("%s\n", buf);
        const char* message = "HTTP/1.1 200 OK\n\r\n\r<h1>Hello World</h1>";
        send(client_fd, message, strlen(message), 0);
        *(client_sockets+i) = client_fd;
        close(client_fd);
    }
    cleanup_sockets();
    close(server_socket);
    return 0;
}
