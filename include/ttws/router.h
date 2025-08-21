#ifndef TTWS_ROUTER_H
#define TTWS_ROUTER_H

#include "server.h"
#include "request.h"
#include "response.h"

typedef int (*RouteHandler)(const TTWS_Request* request, TTWS_Response* response);
typedef struct TTWS_Server TTWS_Server;

void TTWS_AddRoute(TTWS_Server* server, const char* method, const char* path, RouteHandler handler);
void TTWS_PrintRouteTree(const TTWS_Server* server);

#endif
