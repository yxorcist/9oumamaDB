/*

gcc -Wall -Wextra -std=c11 -Iinclude -g \
    src/storage.c \
    src/buffer_pool.c \
    src/page_manager.c \
    tests/test_storage.c \
    -o test_storage \
    && ./test_storage

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "storage.h"

#define TEST_FILE "test_database.db"

static void cleanup(void) { remove(TEST_FILE); }

static void test_empty_database(void) {
  cleanup();

  Storage *storage = storage_create();
  assert(storage != NULL);

  assert(storage_count(storage) == 0);

  assert(storage_save(storage) == 1);

  storage_free(storage);

  storage = storage_create();
  assert(storage != NULL);

  assert(storage_load(storage) == 1);
  assert(storage_count(storage) == 0);

  storage_free(storage);

  cleanup();

  printf("PASS: empty database\n");
}

static void test_save_load(void) {
  cleanup();

  Storage *storage = storage_create();
  assert(storage != NULL);

  assert(storage_create_entry(storage, 1, 100) != NULL);
  assert(storage_create_entry(storage, 2, 200) != NULL);
  assert(storage_create_entry(storage, 3, 300) != NULL);

  assert(storage_count(storage) == 3);
  assert(storage_save(storage) == 1);

  storage_free(storage);

  storage = storage_create();
  assert(storage != NULL);

  assert(storage_load(storage) == 1);
  assert(storage_count(storage) == 3);

  Entry *e0 = storage_get_entry(storage, 0);
  Entry *e1 = storage_get_entry(storage, 1);
  Entry *e2 = storage_get_entry(storage, 2);

  assert(e0 != NULL);
  assert(e1 != NULL);
  assert(e2 != NULL);

  assert(e0->key == 1);
  assert(e0->value == 100);

  assert(e1->key == 2);
  assert(e1->value == 200);

  assert(e2->key == 3);
  assert(e2->value == 300);

  storage_free(storage);

  cleanup();

  printf("PASS: save/load\n");
}

static void test_update_persistence(void) {
  cleanup();

  Storage *storage = storage_create();
  assert(storage != NULL);

  Entry *entry = storage_create_entry(storage, 42, 100);
  assert(entry != NULL);

  entry->value = 999;

  assert(storage_save(storage) == 1);

  storage_free(storage);

  storage = storage_create();
  assert(storage != NULL);

  assert(storage_load(storage) == 1);
  assert(storage_count(storage) == 1);

  entry = storage_get_entry(storage, 0);

  assert(entry != NULL);
  assert(entry->key == 42);
  assert(entry->value == 999);

  storage_free(storage);

  cleanup();

  printf("PASS: update persistence\n");
}

static void test_delete_persistence(void) {
  cleanup();

  Storage *storage = storage_create();
  assert(storage != NULL);

  Entry *a = storage_create_entry(storage, 1, 100);
  Entry *b = storage_create_entry(storage, 2, 200);
  Entry *c = storage_create_entry(storage, 3, 300);

  assert(a != NULL);
  assert(b != NULL);
  assert(c != NULL);

  storage_delete_entry(storage, b);

  assert(storage_count(storage) == 2);

  assert(storage_save(storage) == 1);

  storage_free(storage);

  storage = storage_create();
  assert(storage != NULL);

  assert(storage_load(storage) == 1);
  assert(storage_count(storage) == 2);

  Entry *e0 = storage_get_entry(storage, 0);
  Entry *e1 = storage_get_entry(storage, 1);

  assert(e0 != NULL);
  assert(e1 != NULL);

  assert(e0->key == 1);
  assert(e0->value == 100);

  assert(e1->key == 3);
  assert(e1->value == 300);

  storage_free(storage);

  cleanup();

  printf("PASS: delete persistence\n");
}

static void test_multiple_pages(void) {
  cleanup();

  Storage *storage = storage_create();
  assert(storage != NULL);

  const int count = 1000;

  for (int i = 0; i < count; i++) {
    assert(storage_create_entry(storage, i, i * 10) != NULL);
  }

  assert(storage_count(storage) == count);
  assert(storage_save(storage) == 1);

  storage_free(storage);

  storage = storage_create();
  assert(storage != NULL);

  assert(storage_load(storage) == 1);
  assert(storage_count(storage) == count);

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get_entry(storage, i);

    assert(entry != NULL);
    assert(entry->key == i);
    assert(entry->value == i * 10);
  }

  storage_free(storage);

  cleanup();

  printf("PASS: multiple pages\n");
}

int main(void) {
  test_empty_database();
  test_save_load();
  test_update_persistence();
  test_delete_persistence();
  test_multiple_pages();

  printf("\nAll storage tests passed.\n");

  return 0;
}
