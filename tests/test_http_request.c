#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http_request.h"

static void test_parse_get(void) {
  HTTPRequest request;

  assert(http_request_parse(
      "GET /users/42 HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n",
      &request));

  assert(strcmp(request.method, "GET") == 0);
  assert(strcmp(request.path, "/users/42") == 0);
  assert(strcmp(request.version, "HTTP/1.1") == 0);

  printf("PASS: HTTP GET parsing\n");
}

static void test_parse_post(void) {
  HTTPRequest request;

  assert(http_request_parse(
      "POST /users HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n",
      &request));

  assert(strcmp(request.method, "POST") == 0);
  assert(strcmp(request.path, "/users") == 0);
  assert(strcmp(request.version, "HTTP/1.1") == 0);

  printf("PASS: HTTP POST parsing\n");
}

static void test_invalid_request(void) {
  HTTPRequest request;

  assert(!http_request_parse(
      "garbage",
      &request));

  printf("PASS: invalid HTTP request\n");
}

int main(void) {
  test_parse_get();
  test_parse_post();
  test_invalid_request();

  return 0;
}
