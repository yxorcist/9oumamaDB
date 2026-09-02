#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http_response.h"

static void test_conflict_response(void) {
  char buffer[1024];

  assert(http_response_build(buffer,
                             sizeof(buffer),
                             409,
                             "application/json",
                             "{\"error\":\"conflict\"}"));

  assert(strstr(buffer, "HTTP/1.1 409 Conflict\r\n") != NULL);
  assert(strstr(buffer, "Content-Type: application/json\r\n") != NULL);
  assert(strstr(buffer, "{\"error\":\"conflict\"}") != NULL);

  printf("PASS: 409 conflict response\n");
}

static void test_ok_response(void) {
  char buffer[1024];

  assert(http_response_build(buffer, sizeof(buffer), 200, "text/plain" ,"hello"));

  assert(strstr(buffer, "HTTP/1.1 200 OK\r\n") != NULL);
  assert(strstr(buffer, "Content-Length: 5\r\n") != NULL);
  assert(strstr(buffer, "Content-Type: text/plain\r\n") != NULL);
  assert(strstr(buffer, "\r\n\r\nhello") != NULL);

  printf("PASS: HTTP 200 response\n");
}

static void test_not_found_response(void) {
  char buffer[1024];

  assert(http_response_build(buffer, sizeof(buffer), 404, "text/plain", "not found"));

  assert(strstr(buffer, "HTTP/1.1 404 Not Found\r\n") != NULL);
  assert(strstr(buffer, "Content-Length: 9\r\n") != NULL);

  printf("PASS: HTTP 404 response\n");
}

static void test_small_buffer(void) {
  char buffer[8];

  assert(!http_response_build(buffer, sizeof(buffer), 200, "text/plain", "hello"));

  printf("PASS: HTTP response buffer validation\n");
}

int main(void) {
  test_ok_response();
  test_not_found_response();
  test_small_buffer();
  test_conflict_response();

  return 0;
}
