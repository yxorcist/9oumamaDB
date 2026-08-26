#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_connection.h"

struct HTTPConnection {
  int client_fd;
};

HTTPConnection *http_connection_create(int client_fd) {
  if (client_fd < 0)
    return NULL;

  HTTPConnection *connection = malloc(sizeof(HTTPConnection));

  if (!connection)
    return NULL;

  connection->client_fd = client_fd;

  return connection;
}

void http_connection_free(HTTPConnection *connection) {
  if (!connection)
    return;

  if (connection->client_fd >= 0)
    close(connection->client_fd);

  free(connection);
}

int http_connection_read(HTTPConnection *connection, char *buffer,
                         size_t capacity) {
  if (!connection || !buffer || capacity == 0)
    return -1;

  ssize_t result = recv(connection->client_fd, buffer, capacity, 0);

  if (result < 0)
    return -1;

  return (int)result;
}

int http_connection_write(HTTPConnection *connection, const char *data,
                          size_t length) {
  if (!connection || !data)
    return 0;

  ssize_t result = send(connection->client_fd, data, length, 0);

  return result == (ssize_t)length;
}
