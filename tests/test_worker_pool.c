#include <assert.h>
#include <stdio.h>

#include "db.h"
#include "worker_pool.h"

static void cleanup(void) {
  remove("database.db");
}

/* ==================== Basic Worker Pool ==================== */

static void test_worker_pool(void) {
  cleanup();

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

  cleanup();

  printf("PASS: worker pool executes requests\n");
}

/* ==================== Worker Pool Operations ==================== */

static void test_worker_pool_operations(void) {
  cleanup();

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

  cleanup();

  printf("PASS: worker pool operations\n");
}

/* ==================== Concurrent Worker Pool ==================== */

static void test_worker_pool_concurrent(void) {
  cleanup();

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

  cleanup();

  printf("PASS: concurrent worker pool requests\n");
}

/* ==================== Failure Handling ==================== */

static void test_worker_pool_failures(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  WorkerPool *pool = worker_pool_create(db, 4, 16);
  assert(pool != NULL);

  /* UPDATE nonexistent key -> failure */
  Request update;

  assert(request_init(&update, CMD_UPDATE, 999, 100));
  assert(worker_pool_submit(pool, &update));

  request_wait(&update);

  assert(!update.result);

  request_destroy(&update);

  /* DELETE nonexistent key -> failure */
  Request delete;

  assert(request_init(&delete, CMD_DELETE, 999, 0));
  assert(worker_pool_submit(pool, &delete));

  request_wait(&delete);

  assert(!delete.result);

  request_destroy(&delete);

  /* First INSERT -> success */
  Request insert;

  assert(request_init(&insert, CMD_INSERT, 1, 100));
  assert(worker_pool_submit(pool, &insert));

  request_wait(&insert);

  assert(insert.result);

  request_destroy(&insert);

  /* Duplicate INSERT -> failure */
  Request duplicate;

  assert(request_init(&duplicate, CMD_INSERT, 1, 200));
  assert(worker_pool_submit(pool, &duplicate));

  request_wait(&duplicate);

  assert(!duplicate.result);

  request_destroy(&duplicate);

  worker_pool_free(pool);
  db_free(db);

  cleanup();

  printf("PASS: worker pool failure handling\n");
}

/* ==================== Main ==================== */

int main(void) {
  cleanup();

  test_worker_pool();
  test_worker_pool_operations();
  test_worker_pool_concurrent();
  test_worker_pool_failures();

  cleanup();

  return 0;
}
