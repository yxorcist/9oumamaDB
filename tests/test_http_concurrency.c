#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "http_server.h"
#include "worker_pool.h"

#define PORT 8081
#define CLIENT_COUNT 20

typedef struct {
  int key;
  int value;
} ClientArgs;

static void *client_worker(void *arg) {
  ClientArgs *args = arg;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  assert(fd >= 0);

  struct sockaddr_in address = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
      .sin_port = htons(PORT),
  };

  assert(connect(fd, (struct sockaddr *)&address, sizeof(address)) == 0);

  char body[64];

  int body_length = snprintf(body, sizeof(body), "{\"value\":%d}", args->value);

  char request[256];

  int length = snprintf(request, 
                        sizeof(request),
                        "POST /kv/%d HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Content-Length: %d\r\n"
                        "Content-Type: application/json\r\n"
                        "\r\n"
                        "%s",
                        args->key,
                        body_length,
                        body);

  assert(send(fd, request, length, 0) == length);

  char response[1024];

  ssize_t received = recv(fd, response, sizeof(response) - 1, 0);

  assert(received > 0);

  response[received] = '\0';

  if (strstr(response, "200 OK") == NULL) {
    fprintf(stderr, "CLIENT %d RECEIVED: \n%s\n", args->key, response);
    assert(0);
  }


  close(fd);

  return NULL;
}

static void *server_worker(void *arg) {
  HTTPServer *server = arg;

  http_server_run(server);

  return NULL;
}

int main(void) {
  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 4, 16);
  assert(pool != NULL);

  HTTPServer *server = http_server_create(PORT, pool);
  assert(server != NULL);

  assert(http_server_start(server));

  pthread_t server_thread;

  assert(pthread_create(
      &server_thread,
      NULL,
      server_worker,
      server) == 0);

  sleep(1);

  pthread_t clients[CLIENT_COUNT];
  ClientArgs args[CLIENT_COUNT];

  for (int i = 0; i < CLIENT_COUNT; i++) {
    args[i].key = i + 1;
    args[i].value = (i + 1) * 10;

    assert(pthread_create(
        &clients[i],
        NULL,
        client_worker,
        &args[i]) == 0);
  }

  for (int i = 0; i < CLIENT_COUNT; i++)
    pthread_join(clients[i], NULL);

  for (int i = 0; i < CLIENT_COUNT; i++) {
    int found = 0;
    int value = db_get(db, i + 1, &found);

    assert(found);
    assert(value == (i + 1) * 10);
  }

  http_server_stop(server);

  pthread_join(server_thread, NULL);

  http_server_free(server);
  worker_pool_free(pool);
  db_free(db);

  printf("PASS: concurrent HTTP requests\n");

  return 0;
}
