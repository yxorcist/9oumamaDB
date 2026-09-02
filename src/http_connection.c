#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

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

static int parse_content_length(const char *buffer,
                                size_t header_length,
                                size_t *content_length) {
  if (!buffer || !content_length)
    return 0;

  *content_length = 0;

  const char *current = buffer;
  const char *header_end = buffer + header_length;

  while (current < header_end) {
    const char *line_end = strstr(current, "\r\n");

    if (!line_end || line_end > header_end)
      break;

    static const char header_name[] = "Content-Length:";

    size_t line_length = (size_t)(line_end - current);

    if (line_length >= sizeof(header_name) - 1 &&
        strncasecmp(current,
                    header_name,
                    sizeof(header_name) - 1) == 0) {

      const char *value = current + sizeof(header_name) - 1;

      while (value < line_end &&
             (*value == ' ' || *value == '\t'))
        value++;

      if (value == line_end)
        return 0;

      char *end;
      unsigned long parsed = strtoul(value, &end, 10);

      if (end == value)
        return 0;

      while (end < line_end &&
             (*end == ' ' || *end == '\t'))
        end++;

      if (end != line_end)
        return 0;

      if (parsed > HTTP_REQUEST_MAX)
        return 0;

      *content_length = (size_t)parsed;

      return 1;
    }

    current = line_end + 2;
  }

  /*
   * No Content-Length header.
   * This is valid for requests without bodies.
   */
  return 1;
}

static ssize_t read_http_request(int client_fd,
                                 char *buffer,
                                 size_t capacity) {
  if (client_fd < 0 || !buffer || capacity < 2)
    return -1;

  size_t total = 0;
  size_t expected_total = 0;
  int headers_complete = 0;

  while (total < capacity - 1) {
    ssize_t result = recv(client_fd,
                          buffer + total,
                          capacity - 1 - total,
                          0);

    if (result <= 0)
      return -1;

    total += (size_t)result;
    buffer[total] = '\0';

    if (!headers_complete) {
      char *header_end = strstr(buffer, "\r\n\r\n");

      if (!header_end)
        continue;

      headers_complete = 1;

      size_t header_length =
        (size_t)(header_end - buffer) + 4;

      size_t content_length = 0;

      if (!parse_content_length(buffer,
                                header_length,
                                &content_length))
        return -1;

      if (content_length > capacity - 1 - header_length)
        return -1;

      expected_total = header_length + content_length;

      /*
       * Request has no body, or the entire body was already
       * received with the headers.
       */
      if (total >= expected_total)
        return (ssize_t)expected_total;
    } else {
      if (total >= expected_total)
        return (ssize_t)expected_total;
    }
  }

  return -1;
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

static int json_escape_string(const char *input,
                              char *output,
                              size_t capacity) {
  if (!input || !output || capacity == 0)
    return 0;

  size_t offset = 0;

  for (size_t i = 0; input[i] != '\0'; i++) {
    unsigned char c = (unsigned char)input[i];

    const char *escape = NULL;

    switch (c) {
    case '"':
      escape = "\\\"";
      break;

    case '\\':
      escape = "\\\\";
      break;

    case '\b':
      escape = "\\b";
      break;

    case '\f':
      escape = "\\f";
      break;

    case '\n':
      escape = "\\n";
      break;

    case '\r':
      escape = "\\r";
      break;

    case '\t':
      escape = "\\t";
      break;

    default:
      break;
    }

    if (escape) {
      size_t length = strlen(escape);

      if (offset + length >= capacity)
        return 0;

      memcpy(output + offset, escape, length);
      offset += length;

      continue;
    }

    if (c < 0x20) {
      int written = snprintf(output + offset,
                             capacity - offset,
                             "\\u%04x",
                             c);

      if (written < 0 ||
          (size_t)written >= capacity - offset)
        return 0;

      offset += (size_t)written;
      continue;
    }

    if (offset + 1 >= capacity)
      return 0;

    output[offset++] = (char)c;
  }

  output[offset] = '\0';

  return 1;
}

static int entry_to_json(char *buffer,
                         size_t capacity,
                         const Entry *entry) {
  if (!buffer || !entry || capacity == 0)
    return 0;

  char escaped_nickname[NICKNAME_MAX * 6];

  if (!json_escape_string(entry->nickname,
                          escaped_nickname,
                          sizeof(escaped_nickname)))
    return 0;

  int written = snprintf(buffer,
                         capacity,
                         "{\"key\":%d,\"value\":%d,\"nickname\":\"%s\"}",
                         entry->key,
                         entry->value,
                         escaped_nickname);

  if (written < 0 || (size_t)written >= capacity)
    return 0;

  return 1;
}

static int entries_to_json(char *buffer,
                           size_t capacity,
                           Entry *entries,
                           int count) {
  if (!buffer || !entries || capacity == 0 || count < 0)
    return 0;

  size_t offset = 0;

  int written = snprintf(buffer, capacity, "[");

  if (written < 0 || (size_t)written >= capacity)
    return 0;

  offset += (size_t)written;

  for (int i = 0; i < count; i++) {
    if (i > 0) {
      if (offset + 1 >= capacity)
        return 0;

      buffer[offset++] = ',';
      buffer[offset] = '\0';
    }

    char entry_json[512];

    if (!entry_to_json(entry_json,
                       sizeof(entry_json),
                       &entries[i]))
      return 0;

    size_t length = strlen(entry_json);

    if (offset + length >= capacity)
      return 0;

    memcpy(buffer + offset, entry_json, length);
    offset += length;
    buffer[offset] = '\0';
  }

  if (offset + 1 >= capacity)
    return 0;

  buffer[offset++] = ']';
  buffer[offset] = '\0';

  return 1;
}

void http_connection_handle(WorkerPool *pool, int client_fd) {

  char buffer[HTTP_REQUEST_MAX];

  ssize_t bytes_read =
    read_http_request(client_fd, buffer, sizeof(buffer));

  if (bytes_read <= 0) {
    send_error_response(client_fd, 400, "bad request");
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

  char response_body[8192];

  int status_code;

  if (request.type == CMD_GET) {
    if (request.found) {

      Entry *entry = &request.entries[0];

      if (!entry_to_json(response_body, sizeof(response_body), entry)) {
        snprintf(response_body, sizeof(response_body), 
                 "{\"error\":\"response too large\"}");
        status_code = 500;
      } else {
        status_code = 200;
      }

    } else {
      snprintf(response_body, sizeof(response_body),
               "{\"error\":\"not found\"}");
      status_code = 404;
    }

  } else if (request.type == CMD_TOPK ||
             request.type == CMD_RANGE) {

    if (!entries_to_json(response_body,
                         sizeof(response_body),
                         request.entries,
                         request.entry_count)) {

      snprintf(response_body, sizeof(response_body),
               "{\"error\":\"response too large\"}");

      status_code = 500;
    } else {
      status_code = 200;
    }

  } else if (request.type == CMD_COUNT) {

    snprintf(response_body, sizeof(response_body),
             "{\"count\":%d}",
             request.result);

    status_code = 200;

  } else if (request.result) {

    snprintf(response_body, sizeof(response_body),
             "{\"status\":\"ok\"}");

    status_code = 200;

  } else {

    switch (request.type) {

      case CMD_INSERT:
        snprintf(response_body, sizeof(response_body),
                 "{\"error\":\"key already exists\"}");
        status_code = 409;
        break;

      case CMD_UPDATE:
      case CMD_DELETE:
        snprintf(response_body, sizeof(response_body),
                 "{\"error\":\"not found\"}");
        status_code = 404;
        break;

      case CMD_BEGIN:
        snprintf(response_body, sizeof(response_body),
                 "{\"error\":\"transaction already active\"}");
        status_code = 409;
        break;

      case CMD_COMMIT:
      case CMD_ROLLBACK:
        snprintf(response_body, sizeof(response_body),
                 "{\"error\":\"no active transaction\"}");
        status_code = 409;
        break;

      default:
        snprintf(response_body, sizeof(response_body),
                 "{\"error\":\"operation failed\"}");
        status_code = 500;
        break;
    }
  }

  char response[16384];

  if (http_response_build(response, sizeof(response),
                          status_code, "application/json", response_body)) {
    send_all(client_fd, response, strlen(response));
  }

  request_destroy(&request);
  close(client_fd);
}
