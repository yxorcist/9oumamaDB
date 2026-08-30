#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "http_request.h"

int http_request_parse(const char *data, HTTPRequest *request) {
  if (!data || !request)
    return 0;

  memset(request, 0, sizeof(HTTPRequest));

  const char *line_end = strstr(data, "\r\n");

  if (!line_end)
    return 0;

  if (sscanf(data, "%15s %255s %15s", request->method, request->path, request->version) != 3)
    return 0;

  const char *line = line_end + 2;

  while (*line != '\0') {
    line_end = strstr(line, "\r\n");

    if (!line_end)
      break;

    if (line == line_end)
      break;

    if (request->header_count >= HTTP_MAX_HEADERS)
      return 0;

    char name[HTTP_HEADER_NAME_MAX];
    char value[HTTP_HEADER_VALUE_MAX];

    if (sscanf(line, "%63[^:]: %255[^\r\n]", name, value) != 2)
      return 0;

    strcpy(request->headers[request->header_count].name, name);
    strcpy(request->headers[request->header_count].value, value);

    request->header_count++;

    line = line_end + 2;
  }

  const char *body = strstr(data, "\r\n\r\n");

  if (!body)
    return 1;

  body += 4;

  const char *content_length_string;

  if (!http_request_get_header(request, "Content-Length", &content_length_string))
    return 1;

  char *end;

  long content_length = strtol(content_length_string, &end, 10);

  if (*end != '\0' || content_length < 0)
    return 0;

  if (content_length > HTTP_BODY_MAX - 1)
    return 0;

  size_t available = strlen(body);

  if ((size_t)content_length > available)
    return 0;

  memcpy(request->body, body, (size_t)content_length);

  request->body_length = (size_t)content_length;
  request->body[request->body_length] = '\0';

  return 1;
}

int http_request_get_header(const HTTPRequest *request, const char *name, const char **value) {
  if (!request || !name || !value)
    return 0;

  for (int i = 0; i < request->header_count; i++) {
    if (strcasecmp(request->headers[i].name, name) == 0) {
      *value = request->headers[i].value;
      return 1;
    }
  }

  return 0;
}
