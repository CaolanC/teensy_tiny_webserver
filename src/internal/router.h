#ifndef ROUTER_H
#define ROUTER_H

#include <sys/epoll.h>
#include "ttws/router.h"

typedef struct TTWS_Request TTWS_Request;
typedef struct TTWS_Response TTWS_Response;

typedef struct RouteNode {
    char* value;
    struct RouteNode* next;
    struct RouteNode** children;
    int no_children;
    RouteHandler handler;
} RouteNode;

static RouteNode* create_route_node();
static void add_route_to_children(RouteNode* parent, RouteNode* new_node);
RouteHandler* get_route_handler(const TTWS_Server* server, const TTWS_Request* request);

#endif
