#include <stdlib.h>

#include "bst.h"
#include "db.h"
#include "entry.h"
#include "hash_table.h"

struct DB {
  HashTable *ht;
  BST *tree;

  Entry **entries;
  int count;
  int capacity;
};

static void grow_entries(DB *db) {
  db->capacity *= 2;
  db->entries = realloc(db->entries, db->capacity * sizeof(Entry *));
}

static void add_entry(DB *db, Entry *entry) {
  if (db->count == db->capacity)
    grow_entries(db);

  db->entries[db->count] = entry;
  db->count++;
}

static void remove_entry(DB *db, Entry *entry) {
  for (int i = 0; i < db->count; i++) {
    if (db->entries[i] == entry) {
      db->entries[i] = db->entries[db->count - 1];
      db->count--;
      return;
    }
  }
}

DB *db_create() {
  DB *db = malloc(sizeof(DB));

  db->ht = ht_create(1024);
  db->tree = bst_create();

  db->capacity = 16;
  db->count = 0;
  db->entries = malloc(db->capacity * sizeof(Entry *));

  return db;
}

void db_free(DB *db) {

  ht_free(db->ht);
  bst_free(db->tree);

  for (int i = 0; i < db->count; i++)
    free(db->entries[i]);

  free(db->entries);
  free(db);
}

void db_insert(DB *db, int key, int value) {

  Entry *entry = ht_get(db->ht, key);

  if (entry) {
    entry->value = value;
    return;
  }

  entry = malloc(sizeof(Entry));

  entry->key = key;
  entry->value = value;

  add_entry(db, entry);

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

  remove_entry(db, entry);

  free(entry);
}

void db_range(DB *db, int a, int b) { bst_range(db->tree, a, b); }
