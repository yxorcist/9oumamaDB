#include <stdlib.h>

#include "storage.h"

struct Storage {
  Entry **entries;
  int count;
  int capacity;
};

static void grow(Storage *storage) {
  storage->capacity *= 2;
  storage->entries =
      realloc(storage->entries, storage->capacity * sizeof(Entry *));
}

Storage *storage_create(void) {
  Storage *storage = malloc(sizeof(Storage));

  storage->capacity = 16;
  storage->count = 0;

  storage->entries = malloc(storage->capacity * sizeof(Entry *));

  return storage;
}

Entry *storage_create_entry(Storage *storage, int key, int value) {
  if (storage->count == storage->capacity)
    grow(storage);

  Entry *entry = malloc(sizeof(Entry));

  entry->key = key;
  entry->value = value;

  storage->entries[storage->count++] = entry;

  return entry;
}

void storage_delete_entry(Storage *storage, Entry *entry) {
  for (int i = 0; i < storage->count; i++) {
    if (storage->entries[i] == entry) {
      storage->entries[i] = storage->entries[storage->count--];
      free(entry);
      return;
    }
  }
}

void storage_free(Storage *storage) {
  for (int i = 0; i < storage->count; i++)
    free(storage->entries[i]);

  free(storage->entries);
  free(storage);
}
