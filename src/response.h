#ifndef TTWS_RESPONSE_H
#define TTWS_RESPONSE_H

typedef struct {
    char* body;
    int status;
    char* http_version;
} TTWS_Response;

#endif
