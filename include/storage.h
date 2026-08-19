#ifndef STORAGE_H
#define STORAGE_H

#include "entry.h"

typedef struct Storage Storage;

Storage *storage_create();
void storage_free(Storage *storage);

Entry *storage_create_entry(Storage *storage, int key, int value);
void storage_delete_entry(Storage *storage, Entry *entry);

int storage_save(Storage *storage, const char *filename);
int storage_load(Storage *storage, const char *filename);

int storage_count(Storage *storage);
Entry *storage_get(Storage *storage, int index);

#endif
