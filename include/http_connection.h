#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <stddef.h>

typedef struct HTTPConnection HTTPConnection;

HTTPConnection *http_connection_create(int client_fd);
void http_connection_free(HTTPConnection *connection);

int http_connection_read(HTTPConnection *connection, char *buffer,
                         size_t capacity);

int http_connection_write(HTTPConnection *connection, const char *data,
                          size_t length);

#endif
