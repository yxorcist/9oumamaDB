#include <stdlib.h>
#include <string.h>

#include "http_dispatch.h"

static int parse_key(const char *path, int *key) {
  if (!path || !key)
    return 0;

  if (path[0] != '/')
    return 0;

  char *end;
  long value = strtol(path + 1, &end, 10);

  if (*end != '\0')
    return 0;

  if (value < 0 || value > 2147483647)
    return 0;

  *key = (int)value;

  return 1;
}

int http_request_to_db_request(
    const HTTPRequest *http_request,
    Request *request) {

  if (!http_request || !request)
    return 0;

  int key;

  if (!parse_key(http_request->path, &key))
    return 0;

  CommandType type;
  int value = 0;

  if (strcmp(http_request->method, "GET") == 0) {
    type = CMD_GET;
  } else if (strcmp(http_request->method, "POST") == 0) {
    type = CMD_INSERT;
  } else if (strcmp(http_request->method, "PUT") == 0) {
    type = CMD_UPDATE;
  } else if (strcmp(http_request->method, "DELETE") == 0) {
    type = CMD_DELETE;
  } else {
    return 0;
  }

  if (type == CMD_INSERT || type == CMD_UPDATE) {
    if (http_request->body_length == 0)
      return 0;

    char *end;

    long parsed_value = strtol(
        http_request->body,
        &end,
        10);

    if (*end != '\0')
      return 0;

    value = (int)parsed_value;
  }

  return request_init(
      request,
      type,
      key,
      value);
}
