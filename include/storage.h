#ifndef STORAGE_H
#define STORAGE_H

#include "entry.h"

typedef struct Storage Storage;

Storage *storage_create(void);
void storage_free(Storage *storage);

Entry *storage_create_entry(Storage *storage, int key, int value, const char *nickname);
void storage_delete_entry(Storage *storage, Entry *entry);

void storage_clear(Storage *storage);

int storage_save(Storage *storage);
int storage_load(Storage *storage);

int storage_count(Storage *storage);
Entry *storage_get_entry(Storage *storage, int index);

int storage_allocate_page(Storage *storage);
int storage_free_page(Storage *storage, int page_id);

void storage_discard_pages(Storage *storage);
void storage_set_writes_enabled(Storage *storage, int enabled);

#endif
