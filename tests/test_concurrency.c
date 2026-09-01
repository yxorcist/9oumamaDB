#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "db.h"
#include "entry.h"

#define THREAD_COUNT 4
#define KEYS_PER_THREAD 100

typedef struct {
  DB *db;
  int start_key;
} WorkerArgs;

/* ==================== Concurrent Inserts ==================== */

static void *writer_worker(void *arg) {
  WorkerArgs *args = arg;

  for (int i = 0; i < KEYS_PER_THREAD; i++) {
    int key = args->start_key + i;
    db_insert(args->db, key, i, "");
  }

  return NULL;
}

static void test_concurrent_insert(void) {
  DB *db = db_create();
  assert(db != NULL);

  pthread_t threads[THREAD_COUNT];
  WorkerArgs args[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++) {
    args[i].db = db;
    args[i].start_key = i * KEYS_PER_THREAD;

    assert(pthread_create(&threads[i], NULL, writer_worker, &args[i]) == 0);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == THREAD_COUNT * KEYS_PER_THREAD);

  db_free(db);

  printf("PASS: concurrent inserts\n");
}

/* ==================== Concurrent Reads ==================== */

static void *reader_worker(void *arg) {
  DB *db = arg;

  for (int i = 0; i < 1000; i++) {
    int key = i % (THREAD_COUNT * KEYS_PER_THREAD);

    int found;
    db_get(db, key, &found);
  }

  return NULL;
}

static void test_concurrent_read(void) {
  DB *db = db_create();
  assert(db != NULL);

  for (int i = 0; i < THREAD_COUNT * KEYS_PER_THREAD; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_create(&threads[i], NULL, reader_worker, db) == 0);

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == THREAD_COUNT * KEYS_PER_THREAD);

  db_free(db);

  printf("PASS: concurrent reads\n");
}

/* ==================== Concurrent Updates ==================== */

static void *updater_worker(void *arg) {
  DB *db = arg;

  for (int i = 0; i < 1000; i++) {
    int key = i % (THREAD_COUNT * KEYS_PER_THREAD);
    db_update(db, key, 9999);
  }

  return NULL;
}

static void test_concurrent_update(void) {
  DB *db = db_create();
  assert(db != NULL);

  for (int i = 0; i < THREAD_COUNT * KEYS_PER_THREAD; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_create(&threads[i], NULL, updater_worker, db) == 0);

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  for (int i = 0; i < THREAD_COUNT * KEYS_PER_THREAD; i++) {
    int found;
    int value = db_get(db, i, &found);

    assert(found);
    assert(value == 9999);
  }

  db_free(db);

  printf("PASS: concurrent updates\n");
}

/* ==================== Concurrent Count ==================== */

static void *counter_worker(void *arg) {
  DB *db = arg;

  for (int i = 0; i < 1000; i++)
    db_count(db);

  return NULL;
}

static void test_concurrent_count(void) {
  DB *db = db_create();
  assert(db != NULL);

  for (int i = 0; i < THREAD_COUNT * KEYS_PER_THREAD; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_create(&threads[i], NULL, counter_worker, db) == 0);

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == THREAD_COUNT * KEYS_PER_THREAD);

  db_free(db);

  printf("PASS: concurrent count\n");
}

/* ==================== Concurrent TOPK ==================== */

static void *topk_worker(void *arg) {
  DB *db = arg;

  for (int i = 0; i < 100; i++) {
    Entry results[10];

    int count = db_topk(db, 10, results);

    assert(count >= 0);
    assert(count <= 10);
  }

  return NULL;
}

static void test_concurrent_topk(void) {
  DB *db = db_create();
  assert(db != NULL);

  for (int i = 0; i < THREAD_COUNT * KEYS_PER_THREAD; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_create(&threads[i], NULL, topk_worker, db) == 0);

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == THREAD_COUNT * KEYS_PER_THREAD);

  db_free(db);

  printf("PASS: concurrent TOPK\n");
}

/* ==================== Concurrent Deletes ==================== */

static void *delete_worker(void *arg) {
  WorkerArgs *args = arg;

  for (int i = 0; i < KEYS_PER_THREAD; i++) {
    int key = args->start_key + i;
    db_delete(args->db, key);
  }

  return NULL;
}

static void test_concurrent_delete(void) {
  DB *db = db_create();
  assert(db != NULL);

  int total = THREAD_COUNT * KEYS_PER_THREAD;

  for (int i = 0; i < total; i++)
    db_insert(db, i, i, "");

  assert(db_count(db) == total);

  pthread_t threads[THREAD_COUNT];
  WorkerArgs args[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++) {
    args[i].db = db;
    args[i].start_key = i * KEYS_PER_THREAD;

    assert(pthread_create(&threads[i], NULL, delete_worker, &args[i]) == 0);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == 0);

  for (int i = 0; i < total; i++) {
    int found;

    db_get(db, i, &found);

    assert(!found);
  }

  db_free(db);

  printf("PASS: concurrent deletes\n");
}

/* ==================== Concurrent Mixed Workload ==================== */

static void test_concurrent_mixed(void) {
  DB *db = db_create();
  assert(db != NULL);

  int total = THREAD_COUNT * KEYS_PER_THREAD;

  for (int i = 0; i < total; i++)
    db_insert(db, i, i, "");

  pthread_t readers[THREAD_COUNT];
  pthread_t updaters[THREAD_COUNT];
  pthread_t counters[THREAD_COUNT];
  pthread_t topks[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++) {
    assert(pthread_create(&readers[i], NULL, reader_worker, db) == 0);

    assert(pthread_create(&updaters[i], NULL, updater_worker, db) == 0);

    assert(pthread_create(&counters[i], NULL, counter_worker, db) == 0);

    assert(pthread_create(&topks[i], NULL, topk_worker, db) == 0);
  }

  for (int i = 0; i < THREAD_COUNT; i++) {
    assert(pthread_join(readers[i], NULL) == 0);
    assert(pthread_join(updaters[i], NULL) == 0);
    assert(pthread_join(counters[i], NULL) == 0);
    assert(pthread_join(topks[i], NULL) == 0);
  }

  assert(db_count(db) == total);

  for (int i = 0; i < total; i++) {
    int found;
    int value = db_get(db, i, &found);

    assert(found);
    assert(value == 9999);
  }

  db_free(db);

  printf("PASS: concurrent mixed workload\n");
}

/* ==================== Concurrent Save ==================== */

static void *save_worker(void *arg) {
  DB *db = arg;

  for (int i = 0; i < 100; i++)
    assert(db_save(db));

  return NULL;
}

static void test_concurrent_save(void) {
  DB *db = db_create();
  assert(db != NULL);

  for (int i = 0; i < 100; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++) {
    assert(pthread_create(&threads[i], NULL, save_worker, db) == 0);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == 100);

  db_free(db);

  printf("PASS: concurrent saves\n");
}

static void *delete_and_save_worker(void *arg) {
  WorkerArgs *args = arg;

  for (int i = 0; i < KEYS_PER_THREAD; i++) {
    int key = args->start_key + i;

    db_delete(args->db, key);
    assert(db_save(args->db));
  }

  return NULL;
}

static void test_concurrent_delete_save(void) {
  DB *db = db_create();
  assert(db != NULL);

  int total = THREAD_COUNT * KEYS_PER_THREAD;

  for (int i = 0; i < total; i++)
    db_insert(db, i, i, "");

  pthread_t threads[THREAD_COUNT];
  WorkerArgs args[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; i++) {
    args[i].db = db;
    args[i].start_key = i * KEYS_PER_THREAD;

    assert(pthread_create(&threads[i], NULL, delete_and_save_worker,
                          &args[i]) == 0);
  }

  for (int i = 0; i < THREAD_COUNT; i++)
    assert(pthread_join(threads[i], NULL) == 0);

  assert(db_count(db) == 0);

  db_free(db);

  printf("PASS: concurrent delete + save\n");
}

/* ==================== Main ==================== */

int main(void) {
  test_concurrent_insert();
  test_concurrent_read();
  test_concurrent_update();
  test_concurrent_count();
  test_concurrent_topk();
  test_concurrent_delete();
  test_concurrent_mixed();
  test_concurrent_save();
  test_concurrent_delete_save();

  return 0;
}
