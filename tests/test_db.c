/*

 gcc -Wall -Wextra -std=c11 -Iinclude -g \
    src/hash_table.c \
    src/bst.c \
    src/db.c \
    src/storage.c \
    src/buffer_pool.c \
    src/page_manager.c \
    tests/test_db.c \
    -o test_db \
    && ./test_db

*/

#include <assert.h>
#include <stdio.h>

#include "db.h"

static void test_crud(void) {
  remove("database.db");

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 100);
  db_insert(db, 2, 200);
  db_insert(db, 3, 300);

  assert(db_count(db) == 3);

  int found;

  assert(db_get(db, 1, &found) == 100);
  assert(found == 1);

  assert(db_get(db, 2, &found) == 200);
  assert(found == 1);

  assert(db_get(db, 999, &found) == 0);
  assert(found == 0);

  db_update(db, 2, 999);

  assert(db_get(db, 2, &found) == 999);
  assert(found == 1);

  db_delete(db, 2);

  assert(db_count(db) == 2);

  assert(db_get(db, 2, &found) == 0);
  assert(found == 0);

  db_free(db);

  printf("PASS: DB CRUD\n");
}

static void test_persistence(void) {

  remove("database.db");

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 10, 1000);
  db_insert(db, 20, 2000);
  db_insert(db, 30, 3000);

  assert(db_count(db) == 3);
  assert(db_save(db) == 1);

  db_free(db);

  /*
   * Create a new DB instance and explicitly load
   * the database from disk.
   */
  db = db_create();
  assert(db != NULL);

  assert(db_load(db) == 1);
  assert(db_count(db) == 3);

  int found;

  assert(db_get(db, 10, &found) == 1000);
  assert(found == 1);

  assert(db_get(db, 20, &found) == 2000);
  assert(found == 1);

  assert(db_get(db, 30, &found) == 3000);
  assert(found == 1);

  db_free(db);

  printf("PASS: DB persistence\n");
}

static void test_delete_persistence(void) {

  remove("database.db");

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 100);
  db_insert(db, 2, 200);
  db_insert(db, 3, 300);

  db_delete(db, 2);

  assert(db_count(db) == 2);
  assert(db_save(db) == 1);

  db_free(db);

  db = db_create();
  assert(db != NULL);

  assert(db_load(db) == 1);
  assert(db_count(db) == 2);

  int found;

  assert(db_get(db, 1, &found) == 100);
  assert(found == 1);

  assert(db_get(db, 2, &found) == 0);
  assert(found == 0);

  assert(db_get(db, 3, &found) == 300);
  assert(found == 1);

  db_free(db);

  printf("PASS: DB delete persistence\n");
}

int main(void) {
  test_crud();
  test_persistence();
  test_delete_persistence();

  remove("database.db");

  printf("\nAll DB tests passed.\n");

  return 0;
}
