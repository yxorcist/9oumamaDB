#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "http_dispatch.h"

/* ==================== GET ==================== */

static void test_get(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");
  strcpy(http.path, "/kv/42");

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_GET);
  assert(request.key == 42);
  assert(request.value == 0);

  request_destroy(&request);

  printf("PASS: HTTP GET dispatch\n");
}

/* ==================== POST ==================== */

static void test_post(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "POST");
  strcpy(http.path, "/kv/42");
  strcpy(http.body, "{\"value\":100,\"nickname\":\"alice\"}");
  http.body_length = strlen(http.body);

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_INSERT);
  assert(request.key == 42);
  assert(request.value == 100);
  assert(strcmp(request.nickname, "alice") == 0);

  request_destroy(&request);

  printf("PASS: HTTP POST dispatch\n");
}

/* ==================== PUT ==================== */

static void test_put(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "PUT");
  strcpy(http.path, "/kv/42");
  strcpy(http.body, "{\"value\":200}");
  http.body_length = strlen(http.body);

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_UPDATE);
  assert(request.key == 42);
  assert(request.value == 200);

  request_destroy(&request);

  printf("PASS: HTTP PUT dispatch\n");
}

/* ==================== DELETE ==================== */

static void test_delete(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "DELETE");
  strcpy(http.path, "/kv/42");

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_DELETE);
  assert(request.key == 42);

  request_destroy(&request);

  printf("PASS: HTTP DELETE dispatch\n");
}

/* ==================== TOPK ==================== */

static void test_topk(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");
  strcpy(http.path, "/topk?k=10");

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_TOPK);
  assert(request.k == 10);

  request_destroy(&request);

  printf("PASS: HTTP TOPK dispatch\n");
}

static void test_invalid_topk(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/topk?k=0");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=-1");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=101");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=abc");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=10garbage");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk?k=");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/topk");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid TOPK dispatch\n");
}

/* ==================== COUNT ==================== */

static void test_count(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");
  strcpy(http.path, "/count");

  assert(http_request_to_db_request(&http, &request));
  assert(request.type == CMD_COUNT);

  request_destroy(&request);

  printf("PASS: HTTP COUNT dispatch\n");
}

/* ==================== RANGE ==================== */

static void test_range(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");
  strcpy(http.path, "/range?a=10&b=50");

  assert(http_request_to_db_request(&http, &request));

  assert(request.type == CMD_RANGE);
  assert(request.a == 10);
  assert(request.b == 50);

  request_destroy(&request);

  printf("PASS: HTTP RANGE dispatch\n");
}

static void test_invalid_range(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/range?a=10&b=5");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=abc&b=5");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=1&b=abc");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=1");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?b=5");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=-1&b=5");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=1&b=-5");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/range?a=1&b=5garbage");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid RANGE dispatch\n");
}

/* ==================== Transactions ==================== */

static void test_transaction_routes(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "POST");

  strcpy(http.path, "/transaction/begin");

  assert(http_request_to_db_request(&http, &request));
  assert(request.type == CMD_BEGIN);

  request_destroy(&request);

  strcpy(http.path, "/transaction/commit");

  assert(http_request_to_db_request(&http, &request));
  assert(request.type == CMD_COMMIT);

  request_destroy(&request);

  strcpy(http.path, "/transaction/rollback");

  assert(http_request_to_db_request(&http, &request));
  assert(request.type == CMD_ROLLBACK);

  request_destroy(&request);

  printf("PASS: HTTP transaction dispatch\n");
}

/* ==================== Invalid Keys ==================== */

static void test_invalid_keys(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/kv/abc");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv/-1");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv/");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv/1/extra");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv/2147483648");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv/42garbage");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid key dispatch\n");
}

/* ==================== Invalid POST Bodies ==================== */

static void test_invalid_post_bodies(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "POST");
  strcpy(http.path, "/kv/42");

  http.body[0] = '\0';
  http.body_length = 0;

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":100}");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"nickname\":\"alice\",\"value\":100}");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":abc,\"nickname\":\"alice\"}");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":100,\"nickname\":\"alice\"");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body,
         "{\"value\":100,\"nickname\":\"alice\"}garbage");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid POST bodies\n");
}

/* ==================== Invalid PUT Bodies ==================== */

static void test_invalid_put_bodies(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "PUT");
  strcpy(http.path, "/kv/42");

  http.body[0] = '\0';
  http.body_length = 0;

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":}");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":abc}");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":100");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.body, "{\"value\":100}garbage");
  http.body_length = strlen(http.body);

  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP invalid PUT bodies\n");
}

/* ==================== Integer Boundaries ==================== */

static void test_integer_boundaries(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/kv/0");

  assert(http_request_to_db_request(&http, &request));
  assert(request.key == 0);

  request_destroy(&request);

  strcpy(http.path, "/kv/2147483647");

  assert(http_request_to_db_request(&http, &request));
  assert(request.key == INT_MAX);

  request_destroy(&request);

  strcpy(http.method, "POST");
  strcpy(http.path, "/kv/1");
  strcpy(http.body,
         "{\"value\":2147483647,\"nickname\":\"max\"}");
  http.body_length = strlen(http.body);

  assert(http_request_to_db_request(&http, &request));
  assert(request.value == INT_MAX);

  request_destroy(&request);

  printf("PASS: HTTP integer boundaries\n");
}

/* ==================== Unknown Routes / Methods ==================== */

static void test_unknown_routes(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.method, "GET");

  strcpy(http.path, "/");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/unknown");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/kv");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.path, "/transaction/begin");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP unknown routes\n");
}

static void test_unsupported_methods(void) {
  HTTPRequest http = {0};
  Request request;

  strcpy(http.path, "/kv/42");

  strcpy(http.method, "PATCH");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.method, "OPTIONS");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.method, "HEAD");
  assert(!http_request_to_db_request(&http, &request));

  strcpy(http.method, "POST");
  strcpy(http.path, "/count");
  assert(!http_request_to_db_request(&http, &request));

  printf("PASS: HTTP unsupported methods\n");
}

/* ==================== Main ==================== */

int main(void) {
  test_get();
  test_post();
  test_put();
  test_delete();

  test_topk();
  test_invalid_topk();

  test_count();

  test_range();
  test_invalid_range();

  test_transaction_routes();

  test_invalid_keys();
  test_invalid_post_bodies();
  test_invalid_put_bodies();

  test_integer_boundaries();

  test_unknown_routes();
  test_unsupported_methods();

  return 0;
}
