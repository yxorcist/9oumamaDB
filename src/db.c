#include <stdlib.h>

#include "bst.h"
#include "db.h"
#include "entry.h"
#include "hash_table.h"
#include "storage.h"

struct DB {
  Storage *storage;
  HashTable *ht;
  BST *tree;
};

DB *db_create() {
  DB *db = malloc(sizeof(DB));

  db->storage = storage_create();
  db->ht = ht_create(1024);
  db->tree = bst_create();

  return db;
}

void db_free(DB *db) {

  ht_free(db->ht);
  bst_free(db->tree);

  storage_free(db->storage);

  free(db);
}

void db_insert(DB *db, int key, int value) {

  Entry *entry = ht_get(db->ht, key);

  if (entry) {
    entry->value = value;
    return;
  }

  entry = storage_create_entry(db->storage, key, value);

  ht_insert(db->ht, entry);
  bst_insert(db->tree, entry);
}

int db_get(DB *db, int key, int *found) {

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    *found = 0;
    return 0;
  }

  *found = 1;
  return entry->value;
}

void db_delete(DB *db, int key) {

  Entry *entry = ht_get(db->ht, key);

  if (!entry)
    return;

  ht_delete(db->ht, key);
  bst_delete(db->tree, key);

  storage_delete_entry(db->storage, entry);
}

void db_update(DB *db, int key, int value) {
  Entry *entry = ht_get(db->ht, key);

  if (!entry)
    return;

  entry->value = value;
}

void db_clear(DB *db) {
  storage_clear(db->storage);

  ht_free(db->ht);
  bst_free(db->tree);

  db->ht = ht_create(1024);
  db->tree = bst_create();
}

void db_range(DB *db, int a, int b) { bst_range(db->tree, a, b); }

int db_count(DB *db) { return storage_count(db->storage); }

int db_save(DB *db, const char *filename) {
  return storage_save(db->storage, filename);
}

int db_load(DB *db, const char *filename) {
  db_clear(db);

  if (!storage_load(db->storage, filename))
    return 0;

  int count = storage_count(db->storage);

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get(db->storage, i);

    ht_insert(db->ht, entry);
    bst_insert(db->tree, entry);
  }

  return 1;
}
