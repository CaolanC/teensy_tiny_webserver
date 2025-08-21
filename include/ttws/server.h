#ifndef TTWS_SERVER_H
#define TTWS_SERVER_H

typedef struct TTWS_Server TTWS_Server;
typedef struct TTWS_Response TTWS_Response;

TTWS_Server* TTWS_CreateServer(int port);
void TTWS_StartServer(TTWS_Server* server);
int TTWS_SendFile(TTWS_Response* res, const char* filepath, int status_code);

#endif
