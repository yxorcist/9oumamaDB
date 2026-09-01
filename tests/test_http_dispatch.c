#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "http_dispatch.h"

static void test_range(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "GET");
  strcpy(http.path, "/range?a=10&b=50");

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_RANGE);
  assert(request.a == 10);
  assert(request.b == 50);

  request_destroy(&request);

  printf("PASS: HTTP RANGE dispatch\n");
}

static void test_count(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "GET");
  strcpy(http.path, "/count");

  Request request;

  assert(http_request_to_db_request(&http, &request));
  assert(request.type == CMD_COUNT);

  request_destroy(&request);

  printf("PASS: HTTP COUNT dispatch\n");
}

static void test_invalid_topk(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/topk?k=0");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=101");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=abc");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid TOPK dispatch\n");
}

static void test_topk(void) {
  HTTPRequest http = {0};

  strcpy(http.method, "GET");
  strcpy(http.path, "/topk?k=10");

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_TOPK);
  assert(request.k == 10);

  request_destroy(&request);

  printf("PASS: HTTP TOPK dispatch\n");
}

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
  strcpy(http.body, "{\"value\":100,\"nickname\":\"alice\"}");
  http.body_length = strlen(http.body);

  Request request;

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_INSERT);
  assert(request.key == 42);
  assert(request.value == 100);
  assert(strcmp(request.nickname, "alice") == 0);

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
  test_topk();
  test_invalid_topk();
  test_count();
  test_range();

  return 0;
}
