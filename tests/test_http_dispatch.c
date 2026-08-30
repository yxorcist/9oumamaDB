#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http_dispatch.h"

static void test_get(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "GET");

  strcpy(http.path, "/kv/42");
  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_GET);
  assert(request.key == 42);
  assert(request.value == 0);

  request_destroy(&request);

  printf("PASS: HTTP GET dispatch\n");
}

static void test_post(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "POST");
  strcpy(http.path, "/kv/42");
  strcpy(http.body, "{\"value\":100}");
  http.body_length = strlen(http.body);

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_INSERT);
  assert(request.key == 42);
  assert(request.value == 100);

  request_destroy(&request);

  printf("PASS: HTTP POST dispatch\n");
}

static void test_put(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "PUT");
  strcpy(http.path, "/kv/42");
  strcpy(http.body, "{\"value\":200}");
  http.body_length = strlen(http.body);

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_UPDATE);
  assert(request.key == 42);
  assert(request.value == 200);

  request_destroy(&request);

  printf("PASS: HTTP PUT dispatch\n");
}

static void test_delete(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "DELETE");
  strcpy(http.path, "/kv/42");

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_DELETE);
  assert(request.key == 42);

  request_destroy(&request);

  printf("PASS: HTTP DELETE dispatch\n");
}

int main(void) {
  test_get();
  test_post();
  test_put();
  test_delete();

  return 0;
}
