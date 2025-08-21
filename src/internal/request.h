#ifndef TTWS_REQUEST_H
#define TTWS_REQUEST_H

typedef struct {
    char* method;
    char* path;
    char* version;
} TTWS_Request;

#endif
