#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "http_dispatch.h"
#include "parser.h"
#include "request_queue.h"

static int parse_range(const char *path, int *a, int *b) {
  if (!path || !a || !b)
    return 0;

  const char *prefix = "/range?a=";

  if (strncmp(path, prefix, strlen(prefix)) != 0)
    return 0;

  const char *first = path + strlen(prefix);
  char *end;

  long parsed_a = strtol(first, &end, 10);

  if (end == first)
    return 0;

  if (parsed_a < 0 || parsed_a > INT_MAX)
    return 0;

  const char *middle = "&b=";

  if (strncmp(end, middle, strlen(middle)) != 0)
    return 0;

  const char *second = end + strlen(middle);

  long parsed_b = strtol(second, &end, 10);

  if (end == second)
    return 0;

  if (*end != '\0')
    return 0;

  if (parsed_b < 0 || parsed_b > INT_MAX)
    return 0;

  if (parsed_a > parsed_b)
    return 0;

  *a = (int)parsed_a;
  *b = (int)parsed_b;

  return 1;
}

static int parse_topk(const char *path, int *k) {
  if (!path || !k)
    return 0;

  const char *prefix = "/topk?k=";

  if (strncmp(path, prefix, strlen(prefix)) != 0)
    return 0;

  const char *number = path + strlen(prefix);
  char *end;

  long value = strtol(number, &end, 10);

  if (end == number)
    return 0;

  if (*end != '\0')
    return 0;

  if (value <= 0 || value > REQUEST_RESULT_CAPACITY)
    return 0;

  *k = (int)value;

  return 1;
}

static int parse_key(const char *path, int *key) {
  if (!path || !key)
    return 0;

  const char *prefix = "/kv/";

  if (strncmp(path, prefix, strlen(prefix)) != 0)
    return 0;

  const char *number = path + strlen(prefix);
  char *end;

  long value = strtol(number, &end, 10);

  if (end == number)
    return 0;

  if (*end != '\0')
    return 0;

  if (value < 0 || value > INT_MAX)
    return 0;

  *key = (int)value;

  return 1;
}

static int parse_insert_body(const char *body, int *value, char nickname[NICKNAME_MAX]) {
  if (!body || !value || !nickname)
    return 0;

  long parsed_value;
  char parsed_nickname[NICKNAME_MAX];
  int consumed = 0;

  int matched = sscanf(body,
                       "{\"value\":%ld,\"nickname\":\"%31[^\"]\"}%n",
                       &parsed_value,
                       parsed_nickname,
                       &consumed);

  if (matched != 2)
    return 0;

  if (body[consumed] != '\0')
    return 0;

  if (parsed_value < 0 || parsed_value > INT_MAX)
    return 0;

  *value = (int) parsed_value;
  strcpy(nickname, parsed_nickname);

  return 1;
}

static int parse_value_body(const char *body, int *value) {
  if (!body || !value)
    return 0;

  const char *prefix = "{\"value\":";

  if (strncmp(body, prefix, strlen(prefix)) != 0)
    return 0;

  char *end;
  long parsed_value = strtol(body + strlen(prefix), &end, 10);

  if (end == body + strlen(prefix))
    return 0;

  if (parsed_value < 0 || parsed_value > INT_MAX)
    return 0;

  if (strcmp(end, "}") != 0)
    return 0;

  *value = (int) parsed_value;

  return 1;
}

int http_request_to_db_request(const HTTPRequest *http_request, Request *request) {
  if (!http_request || !request)
    return 0;

  /* Transaction Operations */

  if (strcmp(http_request->method, "POST") == 0) {
    if (strcmp(http_request->path, "/transaction/begin") == 0)
      return request_init(request, CMD_BEGIN, 0, 0);

    if (strcmp(http_request->path, "/transaction/commit") == 0)
      return request_init(request, CMD_COMMIT, 0, 0);

    if (strcmp(http_request->path, "/transaction/rollback") == 0)
      return request_init(request, CMD_ROLLBACK, 0, 0);
  }

  /* Generic GET Operations */

  if (strcmp(http_request->method, "GET") == 0) {

    if (strcmp(http_request->path, "/count") == 0)
      return request_init(request, CMD_COUNT, 0, 0);

    int k;

    if (parse_topk(http_request->path, &k)) {
      if (!request_init(request, CMD_TOPK, 0, 0))
        return 0;

      request->k = k;
      return 1;
    }

    int a, b;

    if (parse_range(http_request->path, &a, &b)) {
      if (!request_init(request, CMD_RANGE, 0, 0))
        return 0;

      request->a = a;
      request->b = b;
      return 1;
    }
  }

  /* /kv/{key} */
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

  if (type == CMD_INSERT)  {
    if (http_request->body_length == 0)
      return 0;

    if (!request_init(request, type, key, 0))
      return 0;

    if (!parse_insert_body(http_request->body,
                           &value,
                           request->nickname)) {
      request_destroy(request);
      return 0;
    }

    request->value = value;

    return 1;
  }

  if (type == CMD_UPDATE) {
    if (http_request->body_length == 0)
      return 0;

    if (!parse_value_body(http_request->body, &value))
      return 0;
  }

  return request_init( request, type, key, value);
}
