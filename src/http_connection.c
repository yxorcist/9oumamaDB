#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "http_connection.h"
#include "http_dispatch.h"
#include "http_response.h"

#define HTTP_REQUEST_MAX 8192

struct HTTPConnection {
  int client_fd;
};

static int send_all(int fd, const char *data, size_t length) {
  size_t sent = 0;

  while (sent < length) {
    ssize_t result = send(fd, data + sent, length - sent, 0);

    if (result <= 0)
      return 0;

    sent += (size_t)result;
  }

  return 1;
}

static void send_error_response(int client_fd, int status_code, const char *body) {

  char response[1024];

  if (!http_response_build( response, 
                           sizeof(response), 
                           status_code, 
                           "text/plain", 
                           body))
    return;

  send_all(client_fd, response, strlen(response));
}

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

void http_connection_handle(WorkerPool *pool, int client_fd) {
  char buffer[HTTP_REQUEST_MAX];

  ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

  if (bytes_read <= 0) {
    close(client_fd);
    return;
  }

  buffer[bytes_read] = '\0';

  HTTPRequest http_request;

  if (!http_request_parse(buffer, &http_request)) {
    send_error_response(client_fd, 400, "bad request");
    close(client_fd);
    return;
  }

  Request request;

  if (!http_request_to_db_request(&http_request, &request)) {
    send_error_response(client_fd, 400, "bad request");
    close(client_fd);
    return;
  }

  if (!worker_pool_submit(pool, &request)) {
    request_destroy(&request);
    send_error_response(client_fd, 500, "internal server error");
    close(client_fd);
    return;
  }

  request_wait(&request);

  char response_body[256];

  int status_code;

  if (request.type == CMD_GET) {
    if (request.found) {
      snprintf(response_body, sizeof(response_body),
              "{\"key\":%d,\"value\":%d}",
               request.key,
               request.result);
      status_code = 200;
    } else {
      snprintf(response_body, sizeof(response_body),
              "{\"error\":\"not found\"}");
      status_code = 404;
    }
  } else if (request.result) {
    snprintf(response_body, sizeof(response_body), "OK");
    status_code = 200;
  } else {
    snprintf(response_body, sizeof(response_body),
            "operation failed");
    status_code = 500;
  }

  char response[1024];

  if (http_response_build(response, sizeof(response),
                          status_code, "application/json", response_body)) {
    send(client_fd, response, strlen(response), 0);
  }

  request_destroy(&request);
  close(client_fd);
}
