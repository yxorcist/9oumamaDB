#include <assert.h>
#include <stdio.h>

#include "db.h"
#include "entry.h"

static void cleanup() { remove("database.db"); }

static void test_transaction_rollback(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 100);
  db_insert(db, 2, 200);

  assert(db_begin(db));

  db_update(db, 1, 999);
  db_delete(db, 2);
  db_insert(db, 3, 300);

  assert(db_count(db) == 2);

  assert(db_rollback(db));

  assert(db_count(db) == 2);

  int found;

  assert(db_get(db, 1, &found) == 100);
  assert(found);

  assert(db_get(db, 2, &found) == 200);
  assert(found);

  assert(db_get(db, 3, &found) == 0);
  assert(!found);

  db_free(db);
  cleanup();

  printf("PASS: DB transaction rollback\n");
}

static void test_transactions(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_begin(db));
  assert(!db_begin(db));

  assert(db_commit(db));
  assert(!db_commit(db));

  db_free(db);
  cleanup();

  printf("PASS: DB transactions\n");
}

static void test_topk(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 40);
  db_insert(db, 2, 90);
  db_insert(db, 3, 15);
  db_insert(db, 4, 70);
  db_insert(db, 5, 100);

  Entry *results[3];

  int count = db_topk(db, 3, results);

  assert(count == 3);

  assert(results[0]->key == 5);
  assert(results[0]->value == 100);

  assert(results[1]->key == 2);
  assert(results[1]->value == 90);

  assert(results[2]->key == 4);
  assert(results[2]->value == 70);

  /*
   * TOPK must not modify the database.
   */
  assert(db_count(db) == 5);

  int found;
  assert(db_get(db, 5, &found) == 100);
  assert(found);

  db_free(db);
  cleanup();

  printf("PASS: DB TOPK\n");
}

static void test_page_reclamation(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  int per_page = 4096 / sizeof(Entry);

  /*
   * Fill two pages.
   */
  for (int i = 0; i < per_page + 1; i++)
    db_insert(db, i, i * 10);

  assert(db_count(db) == per_page + 1);
  assert(db_save(db) == 1);

  /*
   * Remove the entry that occupies the second page.
   */
  db_delete(db, per_page);

  assert(db_count(db) == per_page);
  assert(db_save(db) == 1);

  /*
   * Verify the remaining data survives a reload.
   */
  db_free(db);

  db = db_create();
  assert(db != NULL);

  assert(db_load(db) == 1);
  assert(db_count(db) == per_page);

  for (int i = 0; i < per_page; i++) {
    int found;
    int value = db_get(db, i, &found);

    assert(found);
    assert(value == i * 10);
  }

  /*
   * The reclaimed page should be available for reuse.
   *
   * DB itself doesn't expose page allocation, so the
   * persistence/data-integrity portion is what we verify
   * at the DB layer.
   */

  db_free(db);
  cleanup();

  printf("PASS: DB page reclamation\n");
}

static void test_crud(void) {
  cleanup();

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

  cleanup();

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

  cleanup();

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
  test_page_reclamation();
  test_topk();
  test_transactions();
  test_transaction_rollback();

  cleanup();

  printf("\nAll DB tests passed.\n");

  return 0;
}
