#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

typedef struct HTTPServer HTTPServer;

HTTPServer *http_server_create(int port);
void http_server_free(HTTPServer *server);

int http_server_start(HTTPServer *server);
void http_server_stop(HTTPServer *server);

int http_server_accept(HTTPServer *server);

#endif
