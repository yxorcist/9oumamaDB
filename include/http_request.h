#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>

#define HTTP_METHOD_MAX 16
#define HTTP_PATH_MAX 256
#define HTTP_VERSION_MAX 16

#define HTTP_HEADER_NAME_MAX 64
#define HTTP_HEADER_VALUE_MAX 256
#define HTTP_MAX_HEADERS 32

#define HTTP_BODY_MAX 4096

typedef struct {
  char name[HTTP_HEADER_NAME_MAX];
  char value[HTTP_HEADER_VALUE_MAX];
} HTTPHeader;

typedef struct {
  char method[HTTP_METHOD_MAX];
  char path[HTTP_PATH_MAX];
  char version[HTTP_VERSION_MAX];

  HTTPHeader headers[HTTP_MAX_HEADERS];
  int header_count;

  char body[HTTP_BODY_MAX];
  size_t body_length;
} HTTPRequest;

int http_request_parse(const char *data, HTTPRequest *request);
int http_request_get_header(const HTTPRequest *request, const char *name, const char **value);

#endif
