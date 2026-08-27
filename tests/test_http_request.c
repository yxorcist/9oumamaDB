#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http_request.h"

static void test_parse_body(void) {
  HTTPRequest request;

  assert(http_request_parse(
      "POST /users HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 13\r\n"
      "\r\n"
      "Hello, world!",
      &request));

  assert(request.body_length == 13);
  assert(strcmp(request.body, "Hello, world!") == 0);

  printf("PASS: HTTP body parsing\n");
}

static void test_parse_headers(void) {
  HTTPRequest request;

  assert(http_request_parse(
      "GET /users HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n"
      "\r\n",
      &request));

  assert(request.header_count == 2);

  assert(strcmp(request.headers[0].name, "Host") == 0);
  assert(strcmp(request.headers[0].value, "localhost") == 0);

  assert(strcmp(request.headers[1].name, "Connection") == 0);
  assert(strcmp(request.headers[1].value, "close") == 0);

  printf("PASS: HTTP header parsing\n");
}

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
  test_parse_headers();
  test_parse_body();

  return 0;
}
