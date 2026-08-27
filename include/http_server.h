#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "worker_pool.h"

typedef struct HTTPServer HTTPServer;

HTTPServer *http_server_create(int port, WorkerPool *pool);
void http_server_free(HTTPServer *server);

int http_server_start(HTTPServer *server);
void http_server_stop(HTTPServer *server);

int http_server_accept(HTTPServer *server);
void http_server_run(HTTPServer *server);

#endif
