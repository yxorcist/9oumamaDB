#include <assert.h>
#include <stdio.h>

#include "db.h"
#include "worker_pool.h"

static void test_worker_pool_concurrent(void) {
  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 4, 32);
  assert(pool != NULL);

  const int request_count = 400;

  Request requests[request_count];

  for (int i = 0; i < request_count; i++) {
    assert(request_init(&requests[i], CMD_INSERT, i, i * 10));
    assert(worker_pool_submit(pool, &requests[i]));
  }

  for (int i = 0; i < request_count; i++) {
    request_wait(&requests[i]);
    assert(requests[i].result);
    request_destroy(&requests[i]);
  }

  assert(db_count(db) == request_count);

  for (int i = 0; i < request_count; i++) {
    int found;
    int value = db_get(db, i, &found);

    assert(found);
    assert(value == i * 10);
  }

  worker_pool_free(pool);
  db_free(db);

  printf("PASS: concurrent worker pool requests\n");
}

static void test_worker_pool(void) {
  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 4, 16);
  assert(pool != NULL);

  for (int i = 0; i < 100; i++) {
    Request request;

    assert(request_init(&request, CMD_INSERT, i, i * 10));

    assert(worker_pool_submit(pool, &request));

    request_wait(&request);

    assert(request.result);

    request_destroy(&request);
  }

  assert(db_count(db) == 100);

  for (int i = 0; i < 100; i++) {
    int found;
    int value = db_get(db, i, &found);

    assert(found);
    assert(value == i * 10);
  }

  worker_pool_free(pool);
  db_free(db);

  printf("PASS: worker pool executes requests\n");
}

static void test_worker_pool_operations(void) {
  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 4, 16);
  assert(pool != NULL);

  Request insert;

  assert(request_init(&insert, CMD_INSERT, 1, 100));

  assert(worker_pool_submit(pool, &insert));

  request_wait(&insert);

  assert(insert.result);

  request_destroy(&insert);

  Request update;

  assert(request_init(&update, CMD_UPDATE, 1, 200));

  assert(worker_pool_submit(pool, &update));

  request_wait(&update);

  assert(update.result);

  request_destroy(&update);

  int found;
  int value = db_get(db, 1, &found);

  assert(found);
  assert(value == 200);

  Request get;

  assert(request_init(&get, CMD_GET, 1, 0));

  assert(worker_pool_submit(pool, &get));

  request_wait(&get);

  assert(get.found);
  assert(get.result == 200);

  request_destroy(&get);

  Request delete;

  assert(request_init(&delete, CMD_DELETE, 1, 0));

  assert(worker_pool_submit(pool, &delete));

  request_wait(&delete);

  assert(delete.result);

  request_destroy(&delete);

  db_get(db, 1, &found);
  assert(!found);

  worker_pool_free(pool);
  db_free(db);

  printf("PASS: worker pool operations\n");
}

int main(void) {
  test_worker_pool();
  test_worker_pool_operations();
  test_worker_pool_concurrent();

  return 0;
}
