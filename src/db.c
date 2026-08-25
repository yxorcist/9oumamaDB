#include <bits/pthreadtypes.h>
#include <pthread.h>
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

  pthread_mutex_t mutex;
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

static void db_clear_unlocked(DB *db) {
  storage_clear(db->storage);

  ht_clear(db->ht);
  bst_clear(db->tree);
  heap_clear(db->heap);
}

static int db_persist(DB *db) {
  if (!db)
    return 0;
  return storage_save(db->storage);
}

DB *db_create() {
  DB *db = malloc(sizeof(DB));

  if (!db)
    return NULL;

  db->storage = storage_create();
  db->ht = ht_create(1024);
  db->tree = bst_create();
  db->heap = heap_create(1024);

  db->transaction_entries = NULL;
  db->in_transaction = 0;
  db->transaction_count = 0;

  if (pthread_mutex_init(&db->mutex, NULL) != 0) {
    free(db);
    return NULL;
  }

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
    transaction_clear(db);
  }

  ht_free(db->ht);
  bst_free(db->tree);
  storage_free(db->storage);
  heap_free(db->heap);

  pthread_mutex_destroy(&db->mutex);

  free(db);
}

void db_insert(DB *db, int key, int value) {
  if (!db)
    return;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (entry) {
    entry->value = value;
    pthread_mutex_unlock(&db->mutex);
    return;
  }

  entry = storage_create_entry(db->storage, key, value);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return;
  }

  ht_insert(db->ht, entry);
  bst_insert(db->tree, entry);
  heap_insert(db->heap, entry);

  pthread_mutex_unlock(&db->mutex);
}

int db_get(DB *db, int key, int *found) {

  if (!db || !found)
    return 0;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    *found = 0;
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  *found = 1;
  int value = entry->value;

  pthread_mutex_unlock(&db->mutex);

  return value;
}

void db_delete(DB *db, int key) {

  if (!db)
    return;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return;
  }

  ht_delete(db->ht, key);
  bst_delete(db->tree, key);
  heap_remove(db->heap, entry);
  storage_delete_entry(db->storage, entry);

  pthread_mutex_unlock(&db->mutex);
}

void db_update(DB *db, int key, int value) {

  if (!db)
    return;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (entry)
    entry->value = value;

  pthread_mutex_unlock(&db->mutex);
}

void db_clear(DB *db) {
  if (!db)
    return;

  pthread_mutex_lock(&db->mutex);
  db_clear_unlocked(db);
  pthread_mutex_unlock(&db->mutex);
}

int db_topk(DB *db, int k, Entry *results) {
  if (!db || !results || k <= 0)
    return 0;

  pthread_mutex_lock(&db->mutex);

  int count = storage_count(db->storage);

  if (k > count)
    k = count;

  Heap *heap = heap_create(count);

  if (!heap) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  for (int i = 0; i < count; i++) {
    Entry *entry = storage_get_entry(db->storage, i);

    if (!entry)
      continue;

    if (!heap_insert(heap, entry)) {
      heap_free(heap);
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  int result_count = 0;

  while (result_count < k) {
    Entry *entry = heap_extract_max(heap);

    if (!entry)
      break;

    results[result_count++] = *entry;
  }

  heap_free(heap);

  pthread_mutex_unlock(&db->mutex);

  return result_count;
}

void db_range(DB *db, int a, int b) {

  if (!db)
    return;

  pthread_mutex_lock(&db->mutex);

  bst_range(db->tree, a, b);

  pthread_mutex_unlock(&db->mutex);
}

int db_count(DB *db) {

  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  int count = storage_count(db->storage);

  pthread_mutex_unlock(&db->mutex);

  return count;
}

int db_save(DB *db) {
  if (!db || db->in_transaction)
    return 0;

  pthread_mutex_lock(&db->mutex);
  int result = db_persist(db);
  pthread_mutex_unlock(&db->mutex);

  return result;
}

int db_load(DB *db) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  db_clear_unlocked(db);

  if (!storage_load(db->storage)) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  int result = db_rebuild_indexes(db);

  pthread_mutex_unlock(&db->mutex);

  return result;
}

int db_begin(DB *db) {
  if (!db || db->in_transaction)
    return 0;

  pthread_mutex_lock(&db->mutex);

  if (db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  int count = storage_count(db->storage);

  Entry **snapshot = NULL;

  if (count > 0) {
    snapshot = malloc(count * sizeof(Entry *));

    if (!snapshot) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }

    for (int i = 0; i < count; i++) {
      Entry *entry = storage_get_entry(db->storage, i);

      snapshot[i] = malloc(sizeof(Entry));

      if (!snapshot[i]) {
        for (int j = 0; j < i; j++)
          free(snapshot[j]);

        free(snapshot);
        pthread_mutex_unlock(&db->mutex);
        return 0;
      }

      *snapshot[i] = *entry;
    }
  }

  storage_set_writes_enabled(db->storage, 0);

  db->in_transaction = 1;

  db->transaction_entries = snapshot;
  db->transaction_count = count;
  pthread_mutex_unlock(&db->mutex);

  return 1;
}

int db_commit(DB *db) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  storage_set_writes_enabled(db->storage, 1);

  if (!db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  int result = db_persist(db);

  if (!result) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  transaction_clear(db);
  db->in_transaction = 0;

  pthread_mutex_unlock(&db->mutex);

  return 1;
}

int db_rollback(DB *db) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  if (!db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  db_clear_unlocked(db);

  for (int i = 0; i < db->transaction_count; i++) {
    Entry *snapshot = db->transaction_entries[i];

    if (!storage_create_entry(db->storage, snapshot->key, snapshot->value)) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  if (!db_rebuild_indexes(db)) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  storage_discard_pages(db->storage);

  storage_set_writes_enabled(db->storage, 1);

  transaction_clear(db);
  db->in_transaction = 0;

  pthread_mutex_unlock(&db->mutex);

  return 1;
}
