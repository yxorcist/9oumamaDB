#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>

int http_response_build( char *buffer,
                        size_t capacity,
                        int status_code,
                        const char *content_type,
                        const char *body);

#endif
