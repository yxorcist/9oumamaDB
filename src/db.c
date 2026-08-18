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

void db_range(DB *db, int a, int b) { bst_range(db->tree, a, b); }
