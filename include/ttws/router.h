#ifndef TTWS_ROUTER_H
#define TTWS_ROUTER_H

typedef struct TTWS_Request TTWS_Request;
typedef struct TTWS_Response TTWS_Response;
typedef struct TTWS_Server TTWS_Server;

typedef int (*RouteHandler)(const TTWS_Request* request, TTWS_Response* response);

void TTWS_AddRoute(TTWS_Server* server, const char* method, const char* path, RouteHandler handler);
void TTWS_PrintRouteTree(const TTWS_Server* server);

#endif
