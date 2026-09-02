#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdlib.h>

#include "bst.h"
#include "db.h"
#include "entry.h"
#include "hash_table.h"
#include "heap.h"
#include "request_queue.h"
#include "storage.h"

struct DB {
  Storage *storage;
  HashTable *ht;
  BST *tree;

  Entry **transaction_entries;
  int transaction_count;
  int in_transaction;

  pthread_mutex_t mutex;
};

int db_in_transaction(DB *db) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);
  int result = db->in_transaction;
  pthread_mutex_unlock(&db->mutex);

  return result;
}

static int db_rebuild_indexes(DB *db) {

  if (!db)
    return 0;

  ht_clear(db->ht);
  bst_clear(db->tree);

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
}

static int db_persist(DB *db) {
  if (!db)
    return 0;
  return storage_save(db->storage);
}

DB *db_create(void) {
  DB *db = malloc(sizeof(DB));

  if (!db)
    return NULL;

  db->storage = storage_create();
  db->ht = ht_create(1024);
  db->tree = bst_create();

  db->transaction_entries = NULL;
  db->in_transaction = 0;
  db->transaction_count = 0;

  if (!db->storage || !db->ht || !db->tree) {
    storage_free(db->storage);
    ht_free(db->ht);
    bst_free(db->tree);
    free(db);
    return NULL;
  }

  if (pthread_mutex_init(&db->mutex, NULL) != 0) {
    storage_free(db->storage);
    ht_free(db->ht);
    bst_free(db->tree);
    free(db);
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

  pthread_mutex_destroy(&db->mutex);

  free(db);
}

int db_insert(DB *db, int key, int value, const char *nickname) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (entry) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  entry = storage_create_entry(db->storage, key, value, nickname);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  ht_insert(db->ht, entry);
  bst_insert(db->tree, entry);

  if (!db->in_transaction) {
    if (!db_persist(db)) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&db->mutex);

  return 1;
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

int db_get_entry(DB *db, int key, Entry *result) {
  if (!db || !result)
    return 0;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  *result = *entry;

  pthread_mutex_unlock(&db->mutex);

  return 1;
}

int db_delete(DB *db, int key) {

  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  ht_delete(db->ht, key);
  bst_delete(db->tree, key);
  storage_delete_entry(db->storage, entry);

  if (!db->in_transaction) {
    if (!db_persist(db)) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&db->mutex);

  return 1;
}

int db_update(DB *db, int key, int value) {

  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  Entry *entry = ht_get(db->ht, key);

  if (!entry) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  entry->value = value;

  if (!db->in_transaction) {
    if (!db_persist(db)) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&db->mutex);

  return 1;
}

int db_clear(DB *db) {
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  db_clear_unlocked(db);

  if (!db->in_transaction) {
    if (!db_persist(db)) {
      pthread_mutex_unlock(&db->mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&db->mutex);

  return 1;
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

int db_range(DB *db, int a, int b, Entry *results, int capacity) {

  if (!db || !results || capacity <= 0)
    return 0;

  pthread_mutex_lock(&db->mutex);

  int count = bst_range(db->tree, a, b, results, capacity);

  pthread_mutex_unlock(&db->mutex);

  return count;
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
  if (!db)
    return 0;

  pthread_mutex_lock(&db->mutex);

  if (db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

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
  if (!db)
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

  if (!db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  storage_set_writes_enabled(db->storage, 1);

  if (!db->in_transaction) {
    pthread_mutex_unlock(&db->mutex);
    return 0;
  }

  storage_set_writes_enabled(db->storage, 1);

  int result = db_persist(db);

  if (!result) {
    storage_set_writes_enabled(db->storage, 0);
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

    if (!storage_create_entry(db->storage, snapshot->key, snapshot->value, snapshot->nickname)) {
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


int db_execute_request(DB *db, Request *request) {
  if (!db || !request)
    return 0;

  switch (request->type) {

  case CMD_INSERT:
    request->result =
        db_insert(db, request->key, request->value, request->nickname);
    break;

  case CMD_GET:
    if (db_get_entry(db, request->key, &request->entries[0])) {
        request->found = 1;
        request->entry_count = 1;
        request->result = request->entries[0].value;
    } else {
        request->found = 0;
        request->entry_count = 0;
        request->result = 0;
    }
    break;

  case CMD_UPDATE:
    request->result = db_update(db, request->key, request->value);
    break;

  case CMD_DELETE:
    request->result = db_delete(db, request->key);
    break;

  case CMD_SAVE:
    request->result = db_save(db);
    break;

  case CMD_LOAD:
    request->result = db_load(db);
    break;

  case CMD_CLEAR:
    request->result = db_clear(db);
    break;

  case CMD_BEGIN:
    request->result = db_begin(db);
    break;

  case CMD_COMMIT:
    request->result = db_commit(db);
    break;

  case CMD_ROLLBACK:
    request->result = db_rollback(db);
    break;

  case CMD_TOPK:
    request->entry_count = db_topk(db, request->k, request->entries);
    request->result = request->entry_count >= 0;
    break;

  case CMD_COUNT:
    request->result = db_count(db);
    break;

  case CMD_RANGE:
    request->entry_count = db_range(db, request->a, request->b, request->entries, REQUEST_RESULT_CAPACITY);
    request->result = 1;
    break;

  default:
    request->result = 0;
    break;
  }

  return 1;
}
