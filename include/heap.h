#ifndef HEAP_H
#define HEAP_H

#include "entry.h"

typedef struct Heap Heap;

Heap *heap_create(int capacity);
void heap_free(Heap *heap);

int heap_insert(Heap *heap, Entry *entry);
Entry *heap_peek(Heap *heap);
Entry *heap_extract_max(Heap *heap);

int heap_size(Heap *heap);

#endif
