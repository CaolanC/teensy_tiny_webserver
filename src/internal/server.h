#ifndef SERVER_H
#define SERVER_H

#include <ttws/TTWS.h>
#include "router.h"

typedef struct RouteNode RouteNode;

typedef struct TTWS_Server { // TODO: Add list of static routes/regex 
    int epoll_instance_fd;
    int socket_fd;
    int port_no;
    struct epoll_event events[MAX_SOCKETS];
    RouteNode route_trie_root;
    char** static_routes;
} TTWS_Server;

TTWS_Server* TTWS_CreateServer(int port);
void TTWS_StartServer(TTWS_Server* server);
int TTWS_SendFile(TTWS_Response* res, const char* filepath, int status_code);

#endif
