#include <stdio.h>
#include <stdlib.h>

#include "bst.h"
#include "db.h"
#include "entry.h"
#include "hash_table.h"
#include "heap.h"
#include "storage.h"

struct DB {
  Storage *storage;
  HashTable *ht;
  BST *tree;
  Heap *heap;
};

static int db_rebuild_indexes(DB *db) {
  int count = storage_count(db->storage);

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get_entry(db->storage, i);

    if (!entry)
      return 0;

    ht_insert(db->ht, entry);
    bst_insert(db->tree, entry);
  }

  return 1;
}

DB *db_create() {
  DB *db = malloc(sizeof(DB));

  if (!db)
    return NULL;

  db->storage = storage_create();
  db->ht = ht_create(1024);
  db->tree = bst_create();
  db->heap = heap_create(1024);

  if (!db->storage || !db->ht || !db->tree || !db->heap) {
    db_free(db);
    return NULL;
  }

  if (!storage_load(db->storage)) {
    db_free(db);
    return NULL;
  }

  if (!db_rebuild_indexes(db)) {
    db_free(db);
    return NULL;
  }

  return db;
}

void db_free(DB *db) {

  if (!db)
    return;

  ht_free(db->ht);
  bst_free(db->tree);
  storage_free(db->storage);
  heap_free(db->heap);

  free(db);
}

void db_insert(DB *db, int key, int value) {
  Entry *entry = ht_get(db->ht, key);

  if (entry) {
    entry->value = value;
    return;
  }

  entry = storage_create_entry(db->storage, key, value);

  if (!entry)
    return;

  ht_insert(db->ht, entry);
  bst_insert(db->tree, entry);
  heap_insert(db->heap, entry);
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

  if (!db)
    return;

  Entry *entry = ht_get(db->ht, key);

  if (!entry)
    return;

  ht_delete(db->ht, key);
  bst_delete(db->tree, key);
  heap_remove(db->heap, entry);

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

int db_topk(DB *db, int k, Entry **results) {
  if (!db || !results || k <= 0)
    return 0;

  int count = storage_count(db->storage);

  if (k > count)
    k = count;

  Heap *heap = heap_create(count);

  if (!heap)
    return 0;

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get_entry(db->storage, i);

    if (!entry)
      continue;

    if (!heap_insert(heap, entry)) {
      heap_free(heap);
      return 0;
    }
  }

  int result_count = 0;

  while (result_count < k) {
    Entry *entry = heap_extract_max(heap);

    if (!entry)
      break;

    results[result_count++] = entry;
  }

  heap_free(heap);

  return result_count;
}

void db_range(DB *db, int a, int b) { bst_range(db->tree, a, b); }

int db_count(DB *db) { return storage_count(db->storage); }

int db_save(DB *db) { return storage_save(db->storage); }

int db_load(DB *db) {
  db_clear(db);

  if (!storage_load(db->storage))
    return 0;

  return db_rebuild_indexes(db);
}
