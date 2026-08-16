#include <stdlib.h>

#include "hash_table.h"

typedef struct Node {
  Entry *entry;
  struct Node *next;
} Node;

struct HashTable {
  int size;
  Node **buckets;
};

static int hash(int key, int size) {
  if (key < 0)
    key = -key;
  return key % size;
}

HashTable *ht_create(int size) {
  HashTable *ht = malloc(sizeof(HashTable));

  ht->size = size;
  ht->buckets = calloc(size, sizeof(Node *));

  return ht;
}

void ht_free(HashTable *ht) {

  for (int i = 0; i < ht->size; i++) {
    Node *n = ht->buckets[i];

    while (n) {
      Node *tmp = n;
      n = n->next;
      free(tmp);
    }
  }

  free(ht->buckets);
  free(ht);
}

void ht_insert(HashTable *ht, Entry *entry) {

  int idx = hash(entry->key, ht->size);

  Node *n = ht->buckets[idx];

  while (n) {
    if (n->entry->key == entry->key) {
      n->entry = entry;
      return;
    }
    n = n->next;
  }

  Node *new_node = malloc(sizeof(Node));

  new_node->entry = entry;
  new_node->next = ht->buckets[idx];

  ht->buckets[idx] = new_node;
}

Entry *ht_get(HashTable *ht, int key) {

  int idx = hash(key, ht->size);

  Node *n = ht->buckets[idx];

  while (n) {
    if (n->entry->key == key) {
      return n->entry;
    }
    n = n->next;
  }

  return NULL;
}

void ht_delete(HashTable *ht, int key) {

  int index = hash(key, ht->size);

  Node *n = ht->buckets[index];
  Node *prev = NULL;

  while (n) {

    if (n->entry->key == key) {
      if (prev)
        prev->next = n->next;
      else
        ht->buckets[index] = n->next;
      free(n);
      return;
    }

    prev = n;
    n = n->next;
  }
}
