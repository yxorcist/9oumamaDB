#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "entry.h"

typedef struct HashTable HashTable;

HashTable *ht_create(int size);
void ht_free(HashTable *ht);

int ht_insert(HashTable *ht, Entry *entry);
Entry *ht_get(HashTable *ht, int key);
void ht_delete(HashTable *ht, int key);
void ht_clear(HashTable *ht);

#endif
