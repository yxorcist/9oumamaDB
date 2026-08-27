#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#define HTTP_METHOD_MAX 16
#define HTTP_PATH_MAX 256
#define HTTP_VERSION_MAX 16

typedef struct {
  char method[HTTP_METHOD_MAX];
  char path[HTTP_PATH_MAX];
  char version[HTTP_VERSION_MAX];
} HTTPRequest;

int http_request_parse(const char *data, HTTPRequest *request);

#endif
