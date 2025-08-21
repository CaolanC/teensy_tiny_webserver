#include "server.h"
#include "request.h"
#include "response.h"
#include "router.h"

#include <stdio.h>

int handle(const TTWS_Request* req, TTWS_Response* res) {
    printf("Handling request for path: %s\n", req->path);
    TTWS_SendFile(res, "../index.html", TTWS_STATUS_OK);
    
    return 0;

}

int main() {
    TTWS_Server* server = TTWS_CreateServer(8080);
    TTWS_AddRoute(server, "GET", "/", handle);
    TTWS_StartServer(server);
    return 0;
}
