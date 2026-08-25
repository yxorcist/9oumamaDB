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

  Entry **transaction_entries;
  int transaction_count;
  int in_transaction;
};

static int db_rebuild_indexes(DB *db) {

  if (!db)
    return 0;

  ht_clear(db->ht);
  bst_clear(db->tree);
  heap_clear(db->heap);

  int count = storage_count(db->storage);

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get_entry(db->storage, i);

    if (!entry)
      return 0;

    ht_insert(db->ht, entry);
    bst_insert(db->tree, entry);
    heap_insert(db->heap, entry);
  }

  return 1;
}

static void transaction_clear(DB *db) {
  if (!db)
    return;

  for (int i = 0; i < db->transaction_count; i++)
    free(db->transaction_entries[i]);

  free(db->transaction_entries);

  db->transaction_entries = NULL;
  db->transaction_count = 0;
}

DB *db_create() {
  DB *db = malloc(sizeof(DB));

  if (!db)
    return NULL;

  db->storage = storage_create();
  db->ht = ht_create(1024);
  db->tree = bst_create();
  db->heap = heap_create(1024);
  db->in_transaction = 0;

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

  if (db->in_transaction) {
    storage_discard_pages(db->storage);
    storage_set_writes_enabled(db->storage, 1);
    // transaction_clear(db);
  }

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
  if (!db)
    return;

  storage_clear(db->storage);

  ht_clear(db->ht);
  bst_clear(db->tree);
  heap_clear(db->heap);
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

static int db_persist(DB *db) {
  if (!db)
    return 0;
  return storage_save(db->storage);
}

int db_save(DB *db) {
  if (!db || db->in_transaction)
    return 0;

  return db_persist(db);
}

int db_load(DB *db) {
  db_clear(db);

  if (!storage_load(db->storage))
    return 0;

  return db_rebuild_indexes(db);
}

int db_begin(DB *db) {
  if (!db || db->in_transaction)
    return 0;

  int count = storage_count(db->storage);

  Entry **snapshot = NULL;

  if (count > 0) {
    snapshot = malloc(count * sizeof(Entry *));

    if (!snapshot)
      return 0;

    for (int i = 0; i < count; i++) {
      Entry *entry = storage_get_entry(db->storage, i);

      snapshot[i] = malloc(sizeof(Entry));

      if (!snapshot[i]) {
        for (int j = 0; j < i; j++)
          free(snapshot[j]);

        free(snapshot);
        return 0;
      }

      *snapshot[i] = *entry;
    }
  }

  storage_set_writes_enabled(db->storage, 0);

  db->transaction_entries = snapshot;
  db->transaction_count = count;
  db->in_transaction = 1;

  return 1;
}

int db_commit(DB *db) {
  if (!db || !db->in_transaction)
    return 0;

  storage_set_writes_enabled(db->storage, 1);

  if (!db_persist(db)) {
    storage_set_writes_enabled(db->storage, 0);
    return 0;
  }

  transaction_clear(db);
  db->in_transaction = 0;

  return 1;
}

int db_rollback(DB *db) {
  if (!db || !db->in_transaction)
    return 0;

  storage_clear(db->storage);

  for (int i = 0; i < db->transaction_count; i++) {
    Entry *snapshot = db->transaction_entries[i];

    if (!storage_create_entry(db->storage, snapshot->key, snapshot->value))
      return 0;
  }

  storage_discard_pages(db->storage);

  storage_set_writes_enabled(db->storage, 1);

  if (!db_rebuild_indexes(db))
    return 0;

  transaction_clear(db);
  db->in_transaction = 0;

  return 1;
}

int db_in_transaction(DB *db) { return db && db->in_transaction; }
