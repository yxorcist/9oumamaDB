#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

#include "http_server.h"

static void test_http_server_create(void) {
  HTTPServer *server = http_server_create(8080);

  assert(server != NULL);

  http_server_free(server);

  printf("PASS: HTTP server creation\n");
}

static void test_http_server_start(void) {
  HTTPServer *server = http_server_create(8080);

  assert(server != NULL);
  assert(http_server_start(server));

  http_server_stop(server);
  http_server_free(server);

  printf("PASS: HTTP server start/stop\n");
}

static void test_http_server_accept(void) {
  HTTPServer *server = http_server_create(8080);

  assert(server != NULL);
  assert(http_server_start(server));

  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  assert(client_fd >= 0);

  struct sockaddr_in address = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
      .sin_port = htons(8080),
  };

  assert(connect(client_fd, (struct sockaddr *)&address, sizeof(address)) == 0);

  int server_client_fd = http_server_accept(server);

  assert(server_client_fd >= 0);

  close(server_client_fd);
  close(client_fd);

  http_server_free(server);

  printf("PASS: HTTP server accept\n");
}

int main(void) {
  test_http_server_create();
  test_http_server_start();
  test_http_server_accept();

  return 0;
}
