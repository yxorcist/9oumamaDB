#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "entry.h"
#include "http_connection.h"
#include "worker_pool.h"

typedef struct {
  WorkerPool *pool;
  int fd;
} ServerArgs;

static void cleanup(void) {
  remove("database.db");
}

static void sleep_short(void) {
  struct timespec delay = {
      .tv_sec = 0,
      .tv_nsec = 20 * 1000 * 1000
  };

  nanosleep(&delay, NULL);
}

static int send_all_test(int fd, const char *data, size_t length) {
  size_t sent = 0;

  while (sent < length) {
    ssize_t result = send(fd, data + sent, length - sent, 0);

    if (result <= 0)
      return 0;

    sent += (size_t)result;
  }

  return 1;
}

static void *server_worker(void *arg) {
  ServerArgs *args = arg;

  http_connection_handle(args->pool, args->fd);

  return NULL;
}

static void test_fragmented_http_request(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 2, 16);
  assert(pool != NULL);

  int sockets[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

  ServerArgs args = {
      .pool = pool,
      .fd = sockets[1]
  };

  pthread_t server_thread;

  assert(pthread_create(&server_thread,
                        NULL,
                        server_worker,
                        &args) == 0);

  const char *body =
      "{\"value\":123,\"nickname\":\"split\"}";

  char headers[512];

  int header_length =
      snprintf(headers,
               sizeof(headers),
               "POST /kv/77 HTTP/1.1\r\n"
               "Host: localhost\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: %zu\r\n"
               "\r\n",
               strlen(body));

  assert(header_length > 0);
  assert((size_t)header_length < sizeof(headers));

  /*
   * Deliberately split the HTTP headers.
   */
  size_t header_split = (size_t)header_length / 2;

  assert(send_all_test(sockets[0],
                       headers,
                       header_split));

  sleep_short();

  assert(send_all_test(sockets[0],
                       headers + header_split,
                       (size_t)header_length - header_split));

  sleep_short();

  /*
   * Deliberately split the HTTP body.
   */
  size_t body_length = strlen(body);
  size_t body_split = body_length / 2;

  assert(send_all_test(sockets[0],
                       body,
                       body_split));

  sleep_short();

  assert(send_all_test(sockets[0],
                       body + body_split,
                       body_length - body_split));

  /*
   * Read the server response until http_connection_handle()
   * closes its end of the socket.
   */
  char response[4096];
  size_t total = 0;

  while (total < sizeof(response) - 1) {
    ssize_t result =
        recv(sockets[0],
             response + total,
             sizeof(response) - 1 - total,
             0);

    if (result == 0)
      break;

    assert(result > 0);

    total += (size_t)result;
  }

  response[total] = '\0';

  assert(pthread_join(server_thread, NULL) == 0);

  assert(strstr(response, "200") != NULL);
  assert(strstr(response, "{\"status\":\"ok\"}") != NULL);

  /*
   * Verify that the fragmented request actually reached
   * the database correctly.
   */
  Entry entry;

  assert(db_get_entry(db, 77, &entry));
  assert(entry.key == 77);
  assert(entry.value == 123);
  assert(strcmp(entry.nickname, "split") == 0);

  close(sockets[0]);

  worker_pool_free(pool);
  db_free(db);

  cleanup();

  printf("PASS: fragmented HTTP request\n");
}

int main(void) {
  test_fragmented_http_request();

  return 0;
}
