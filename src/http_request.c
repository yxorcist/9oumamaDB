#include <stdio.h>

#include "http_request.h"

int http_request_parse(const char *data, HTTPRequest *request) {
  if (!data || !request)
    return 0;

  int fields = sscanf(data, "%15s %255s %15s", request->method, request->path, request->version);

  return fields == 3;
}
