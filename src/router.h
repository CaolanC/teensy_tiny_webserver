#ifndef ROUTER_H
#define ROUTER_H

#include "server.h"
#include "request.h"
#include "response.h"

typedef int (*RouteHandler)(const TTWS_Request* request, TTWS_Response* response);
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
void TTWS_AddRoute(TTWS_Server* server, const char* method, const char* path, RouteHandler handler);
void TTWS_PrintRouteTree(const TTWS_Server* server);

#endif
