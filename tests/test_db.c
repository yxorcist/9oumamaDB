#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "entry.h"

static void cleanup() { remove("database.db"); }

static void test_nickname_rollback(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 10, 1000, "charlie"));

  assert(db_begin(db));

  assert(db_update(db, 10, 5000));

  assert(db_rollback(db));

  Entry entry;

  assert(db_get_entry(db, 10, &entry));
  assert(entry.value == 1000);
  assert(strcmp(entry.nickname, "charlie") == 0);

  db_free(db);
  cleanup();

  printf("PASS: DB nickname rollback\n");
}

static void test_nickname_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 7, 900, "bob"));
  assert(db_save(db));

  db_free(db);

  db = db_create();
  assert(db != NULL);

  Entry entry;

  assert(db_get_entry(db, 7, &entry));
  assert(entry.key == 7);
  assert(entry.value == 900);
  assert(strcmp(entry.nickname, "bob") == 0);

  db_free(db);
  cleanup();

  printf("PASS: DB nickname persistence\n");
}

static void test_nickname(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 42, 1500, "alice"));

  Entry entry;

  assert(db_get_entry(db, 42, &entry));
  assert(entry.key == 42);
  assert(entry.value == 1500);
  assert(strcmp(entry.nickname, "alice") == 0);

  /*
   * Updating the score must not modify the nickname.
   */
  assert(db_update(db, 42, 2000));

  assert(db_get_entry(db, 42, &entry));
  assert(entry.value == 2000);
  assert(strcmp(entry.nickname, "alice") == 0);

  db_free(db);
  cleanup();

  printf("PASS: DB nickname\n");
}

static void test_transaction_exit_without_commit(void) {
  cleanup();

  /* Create and persist initial state. */
  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 100, "");
  assert(db_save(db) == 1);

  db_free(db);

  /* Modify inside a transaction, then destroy DB without committing. */
  db = db_create();
  assert(db != NULL);

  assert(db_begin(db) == 1);

  db_update(db, 1, 999);

  int found;
  assert(db_get(db, 1, &found) == 999);
  assert(found);

  db_free(db);

  /* Reopen: the uncommitted value must not be on disk. */
  db = db_create();
  assert(db != NULL);

  int value = db_get(db, 1, &found);

  assert(found);
  assert(value == 100);

  db_free(db);

  printf("PASS: transaction exit without commit\n");

  remove("database.db");
}

static void test_transaction_rollback(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  db_insert(db, 1, 100, "");
  db_insert(db, 2, 200, "");

  assert(db_begin(db));

  db_update(db, 1, 999);
  db_delete(db, 2);
  db_insert(db, 3, 300, "");

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

  db_insert(db, 1, 40, "one");
  db_insert(db, 2, 90, "two");
  db_insert(db, 3, 152, "three");
  db_insert(db, 4, 702, "four");
  db_insert(db, 5, 100, "five");

  Entry results[3];

  int count = db_topk(db, 3, results);

  assert(count == 3);

  assert(results[0].key == 4);
  assert(results[0].value == 702);
  assert(strcmp(results[0].nickname, "four") == 0);

  assert(results[1].key == 3);
  assert(results[1].value == 152);
  assert(strcmp(results[1].nickname, "three") == 0);

  assert(results[2].key == 5);
  assert(results[2].value == 100);
  assert(strcmp(results[2].nickname, "five") == 0);

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
    db_insert(db, i, i * 10, "");

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

  db_insert(db, 1, 100, "");
  db_insert(db, 2, 200, "");
  db_insert(db, 3, 300, "");

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

  db_insert(db, 10, 1000, "");
  db_insert(db, 20, 2000, "");
  db_insert(db, 30, 3000, "");

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

  db_insert(db, 1, 100, "");
  db_insert(db, 2, 200, "");
  db_insert(db, 3, 300, "");

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
  test_transaction_exit_without_commit();
  test_nickname();
  test_nickname_persistence();
  test_nickname_rollback();

  cleanup();

  printf("\nAll DB tests passed.\n");

  return 0;
}
