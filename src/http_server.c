#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "http_connection.h"
#include "http_server.h"

struct HTTPServer {
  int port;
  int server_fd;
  WorkerPool *pool;
  int running;
};

struct ConnectionArgs {
  WorkerPool *pool;
  int client_fd;
};

static void *connection_worker(void *arg) {

  struct ConnectionArgs *args = arg;

  http_connection_handle(args->pool, args->client_fd);

  free(args);

  return NULL;
}

HTTPServer *http_server_create(int port, WorkerPool *pool) {
  if (port <= 0 || port > 65535 || !pool)
    return NULL;

  HTTPServer *server = malloc(sizeof(HTTPServer));

  if (!server)
    return NULL;

  server->port = port;
  server->server_fd = -1;
  server->pool = pool;
  server->running = 0;

  return server;
}

void http_server_free(HTTPServer *server) {
  if (!server)
    return;

  http_server_stop(server);
  free(server);
}

int http_server_start(HTTPServer *server) {
  if (!server)
    return 0;

  server->server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server->server_fd < 0)
    return 0;

  int reuse = 1;

  if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                 sizeof(reuse)) < 0) {
    close(server->server_fd);
    server->server_fd = -1;
    return 0;
  }

  struct sockaddr_in address = {.sin_family = AF_INET,
                                .sin_addr.s_addr = htonl(INADDR_ANY),
                                .sin_port = htons(server->port)};

  if (bind(server->server_fd, (struct sockaddr *)&address, sizeof(address)) <
      0) {
    close(server->server_fd);
    server->server_fd = -1;
    return 0;
  }

  if (listen(server->server_fd, 16) < 0) {
    close(server->server_fd);
    server->server_fd = -1;
    return 0;
  }

  server->running = 1;

  return 1;
}

void http_server_stop(HTTPServer *server) {
  if (!server)
    return;

  server->running = 0;

  if (server->server_fd >= 0) {
    shutdown(server->server_fd, SHUT_RDWR);
    close(server->server_fd);
    server->server_fd = -1;
  }
}

int http_server_accept(HTTPServer *server) {
  if (!server || server->server_fd < 0)
    return -1;

  struct sockaddr_in client;
  socklen_t client_len = sizeof(client);

  return accept(server->server_fd, (struct sockaddr *)&client, &client_len);
}

void http_server_run(HTTPServer *server) {
  if (!server)
    return;

  while (server->running) {
    int client_fd = http_server_accept(server);

    if (client_fd < 0)
      break;

    struct ConnectionArgs *args = malloc(sizeof(struct ConnectionArgs));

    if (!args) {
      close(client_fd);
      continue;
    }

    args->pool = server->pool;
    args->client_fd = client_fd;

    pthread_t thread;

    if (pthread_create(&thread, NULL, connection_worker, args) != 0) {
      free(args);
      close(client_fd);
      continue;
    }

    pthread_detach(thread);
  }
}
