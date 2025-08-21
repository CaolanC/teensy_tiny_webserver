#ifndef TTWS_RESPONSE_H
#define TTWS_RESPONSE_H

typedef struct TTWS_Response {
    char* body;
    int status;
    char* http_version;
} TTWS_Response;

#endif
