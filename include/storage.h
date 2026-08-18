#ifndef STORAGE_H
#define STORAGE_H

#include "entry.h"

typedef struct Storage Storage;

Storage *storage_create();
void storage_free(Storage *storage);

Entry *storage_create_entry(Storage *storage, int key, int value);
void storage_delete_entry(Storage *storage, Entry *entry);

#endif
