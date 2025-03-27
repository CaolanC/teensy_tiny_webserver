#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed to create socket");
        exit(1);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(fd, (struct sockaddr*) &addr, sizeof(addr));
    listen(fd, 5);
    int size = 1024;
    char buf[size];
    memset(buf, 0, size);
    for(int i = 0; i < 5; i++) {
        int client_fd = accept(fd, 0, 0);
        recv(client_fd, buf, size, 0);
        printf("%s\n", buf);
        const char* message = "HTTP/1.1 200 OK\n\r\n\r<h1>Hello World</h1>";
        send(client_fd, message, strlen(message), 0);
        close(client_fd);
    }
    close(fd);
    return 0;
}
