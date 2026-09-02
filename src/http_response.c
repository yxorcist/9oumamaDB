#include <stdio.h>
#include <string.h>

#include "http_response.h"

static const char *status_text(int status_code) {

  switch (status_code) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 400:
    return "Bad Request";
  case 404:
    return "Not Found";
  case 409:
    return "Conflict";
  case 500:
    return "Internal Server Error";
  default:
    return NULL;
  }

}

int http_response_build( char *buffer,
                        size_t capacity,
                        int status_code,
                        const char *content_type,
                        const char *body) {

  if (!buffer || capacity == 0 || !content_type || !body)
    return 0;

  const char *text = status_text(status_code);

  if (!text)
    return 0;

  int written = snprintf(
    buffer,
    capacity,
    "HTTP/1.1 %d %s\r\n"
    "Content-Length: %zu\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s",
    status_code,
    text,
    strlen(body),
    content_type,
    body
  );

  if (written < 0 || (size_t)written >= capacity)
    return 0;

  return 1;
}

