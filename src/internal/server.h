#ifndef TTWS_SERVER_H
#define TTWS_SERVER_H

#include <ttws/TTWS.h>
#include "response.h"

typedef struct TTWS_Server TTWS_Server;

TTWS_Server* TTWS_CreateServer(int port);
void TTWS_StartServer(TTWS_Server* server);
int TTWS_SendFile(TTWS_Response* res, const char* filepath, int status_code);

#endif
