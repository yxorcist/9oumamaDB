#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "entry.h"

static void cleanup(void) {
  remove("database.db");
}

static void corrupt_database_file(void) {
  FILE *file = fopen("database.db", "r+b");
  assert(file != NULL);

  unsigned char garbage[8] = {0};

  assert(fwrite(garbage, 1, sizeof(garbage), file) == sizeof(garbage));
  assert(fflush(file) == 0);

  fclose(file);
}

static void test_failed_load_preserves_live_database(void) {
  remove("database.db");

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 1, 100, "one"));
  assert(db_insert(db, 2, 200, "two"));

  Entry entry;

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 100);

  /*
   * Damage the persisted database header.
   *
   * The live in-memory DB should remain valid even though
   * the next load attempt fails.
   */
  corrupt_database_file();

  assert(!db_load(db));

  /*
   * Atomic LOAD guarantee:
   * failed disk loading must not destroy live state.
   */
  assert(db_count(db) == 2);

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 100);
  assert(strcmp(entry.nickname, "one") == 0);

  assert(db_get_entry(db, 2, &entry));
  assert(entry.value == 200);
  assert(strcmp(entry.nickname, "two") == 0);

  db_free(db);

  remove("database.db");

  printf("PASS: failed load preserves live database\n");
}

static void test_clear_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 1, 100, "one"));
  assert(db_insert(db, 2, 200, "two"));
  assert(db_insert(db, 3, 300, "three"));

  assert(db_count(db) == 3);

  assert(db_clear(db));
  assert(db_count(db) == 0);

  db_free(db);

  db = db_create();
  assert(db != NULL);

  assert(db_count(db) == 0);

  int found;

  db_get(db, 1, &found);
  assert(!found);

  db_get(db, 2, &found);
  assert(!found);

  db_get(db, 3, &found);
  assert(!found);

  db_free(db);
  cleanup();

  printf("PASS: clear persistence\n");
}

/* ==================== INSERT Persistence ==================== */

static void test_insert_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 101, 777, "persistent"));
  assert(db_count(db) == 1);

  db_free(db);

  db = db_create();
  assert(db != NULL);

  Entry entry;

  assert(db_get_entry(db, 101, &entry));
  assert(entry.key == 101);
  assert(entry.value == 777);
  assert(strcmp(entry.nickname, "persistent") == 0);
  assert(db_count(db) == 1);

  db_free(db);
  cleanup();

  printf("PASS: insert persistence\n");
}

/* ==================== UPDATE Persistence ==================== */

static void test_update_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 10, 100, "alice"));
  assert(db_update(db, 10, 999));

  db_free(db);

  db = db_create();
  assert(db != NULL);

  Entry entry;

  assert(db_get_entry(db, 10, &entry));

  assert(entry.key == 10);
  assert(entry.value == 999);

  /* UPDATE changes only the value. */
  assert(strcmp(entry.nickname, "alice") == 0);

  db_free(db);
  cleanup();

  printf("PASS: update persistence\n");
}

/* ==================== DELETE Persistence ==================== */

static void test_delete_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 20, 200, "delete-me"));

  assert(db_delete(db, 20));
  assert(db_count(db) == 0);

  db_free(db);

  db = db_create();
  assert(db != NULL);

  int found;

  db_get(db, 20, &found);

  assert(!found);
  assert(db_count(db) == 0);

  db_free(db);
  cleanup();

  printf("PASS: delete persistence\n");
}

/* ==================== Multiple Entries ==================== */

static void test_multiple_entry_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 1, 40, "one"));
  assert(db_insert(db, 2, 200, "two"));
  assert(db_insert(db, 3, 100, "three"));

  assert(db_count(db) == 3);

  db_free(db);

  db = db_create();
  assert(db != NULL);

  assert(db_count(db) == 3);

  Entry entry;

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 40);
  assert(strcmp(entry.nickname, "one") == 0);

  assert(db_get_entry(db, 2, &entry));
  assert(entry.value == 200);
  assert(strcmp(entry.nickname, "two") == 0);

  assert(db_get_entry(db, 3, &entry));
  assert(entry.value == 100);
  assert(strcmp(entry.nickname, "three") == 0);

  db_free(db);
  cleanup();

  printf("PASS: multiple entry persistence\n");
}

/* ==================== Transaction Commit ==================== */

static void test_commit_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_begin(db));

  assert(db_insert(db, 1, 100, "one"));
  assert(db_insert(db, 2, 200, "two"));

  assert(db_commit(db));

  db_free(db);

  db = db_create();
  assert(db != NULL);

  assert(db_count(db) == 2);

  Entry entry;

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 100);
  assert(strcmp(entry.nickname, "one") == 0);

  assert(db_get_entry(db, 2, &entry));
  assert(entry.value == 200);
  assert(strcmp(entry.nickname, "two") == 0);

  db_free(db);
  cleanup();

  printf("PASS: committed transaction persistence\n");
}

/* ==================== Transaction Rollback ==================== */

static void test_rollback_persistence(void) {
  cleanup();

  DB *db = db_create();
  assert(db != NULL);

  assert(db_insert(db, 1, 100, "original"));

  assert(db_begin(db));

  assert(db_insert(db, 2, 200, "temporary"));
  assert(db_update(db, 1, 999));

  assert(db_rollback(db));

  assert(db_count(db) == 1);

  Entry entry;

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 100);
  assert(strcmp(entry.nickname, "original") == 0);

  int found;

  db_get(db, 2, &found);
  assert(!found);

  db_free(db);

  /*
   * Restart to prove rollback did not leak transaction state
   * into persistent storage.
   */
  db = db_create();
  assert(db != NULL);

  assert(db_count(db) == 1);

  assert(db_get_entry(db, 1, &entry));
  assert(entry.value == 100);
  assert(strcmp(entry.nickname, "original") == 0);

  db_get(db, 2, &found);
  assert(!found);

  db_free(db);
  cleanup();

  printf("PASS: rolled back transaction not persisted\n");
}

/* ==================== Main ==================== */

int main(void) {
  cleanup();

  test_insert_persistence();
  test_update_persistence();
  test_delete_persistence();
  test_multiple_entry_persistence();
  test_commit_persistence();
  test_rollback_persistence();
  test_clear_persistence();
  test_failed_load_preserves_live_database();

  cleanup();

  return 0;
}
