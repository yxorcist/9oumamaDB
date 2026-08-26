#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_connection.h"

static void test_connection_read(void) {
  int sockets[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

  HTTPConnection *connection = http_connection_create(sockets[0]);

  assert(connection != NULL);

  const char *message = "GET / HTTP/1.1\r\n";

  assert(send(sockets[1], message, strlen(message), 0) ==
         (ssize_t)strlen(message));

  char buffer[128];

  int bytes = http_connection_read(connection, buffer, sizeof(buffer));

  assert(bytes == (int)strlen(message));

  buffer[bytes] = '\0';

  assert(strcmp(buffer, message) == 0);

  close(sockets[1]);
  http_connection_free(connection);

  printf("PASS: HTTP connection read\n");
}

static void test_connection_write(void) {
  int sockets[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

  HTTPConnection *connection = http_connection_create(sockets[0]);

  assert(connection != NULL);

  const char *message = "HTTP/1.1 200 OK\r\n";

  assert(http_connection_write(connection, message, strlen(message)));

  char buffer[128];

  ssize_t bytes = recv(sockets[1], buffer, sizeof(buffer) - 1, 0);

  assert(bytes == (ssize_t)strlen(message));

  buffer[bytes] = '\0';

  assert(strcmp(buffer, message) == 0);

  close(sockets[1]);
  http_connection_free(connection);

  printf("PASS: HTTP connection write\n");
}

int main(void) {
  test_connection_read();
  test_connection_write();

  return 0;
}
