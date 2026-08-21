#include <stdio.h>
#include <stdlib.h>

#include "storage.h"
#include "storage_format.h"

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

      free(entry);

      for (int j = i; j < storage->count - 1; j++) {
        storage->entries[j] = storage->entries[j + 1];
      }

      storage->count--;
      storage->entries[storage->count] = NULL;
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

int storage_save(Storage *storage, const char *filename) {
  FILE *file = fopen(filename, "wb");

  if (!file)
    return 0;

  DatabaseHeader header;

  header.magic = DB_MAGIC;
  header.version = DB_VERSION;
  header.count = storage->count;

  if (fwrite(&header, sizeof(DatabaseHeader), 1, file) != 1) {
    fclose(file);
    return 0;
  }

  for (int i = 0; i < storage->count; i++) {
    Entry *entry = storage->entries[i];

    if (fwrite(entry, sizeof(Entry), 1, file) != 1) {
      fclose(file);
      return 0;
    }
  }

  fclose(file);
  return 1;
}

int storage_load(Storage *storage, const char *filename) {
  FILE *file = fopen(filename, "rb");

  if (!file)
    return 0;

  DatabaseHeader header;

  if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1) {
    fclose(file);
    return 0;
  }

  if (header.magic != DB_MAGIC) {
    fclose(file);
    return 0;
  }

  if (header.version != DB_VERSION) {
    fclose(file);
    return 0;
  }

  for (int i = 0; i < header.count; i++) {
    Entry entry;

    if (fread(&entry, sizeof(entry), 1, file) != 1) {
      fclose(file);
      return 0;
    }

    storage_create_entry(storage, entry.key, entry.value);
  }

  fclose(file);

  return 1;
}

int storage_count(Storage *storage) { return storage->count; }

Entry *storage_get(Storage *storage, int index) {
  if (index < 0 || index >= storage->count)
    return NULL;

  return storage->entries[index];
}
