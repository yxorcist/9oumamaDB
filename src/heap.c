#include <stdlib.h>

#include "heap.h"

struct Heap {
  Entry **entries;
  int size;
  int capacity;
};

static void swap(Entry **a, Entry **b) {
  Entry *tmp = *a;
  *a = *b;
  *b = tmp;
}

static void sift_up(Heap *heap, int index) {
  while (index > 0) {
    int parent = (index - 1) / 2;

    if (heap->entries[parent]->value >= heap->entries[index]->value)
      break;

    swap(&heap->entries[parent], &heap->entries[index]);
    index = parent;
  }
}

static void sift_down(Heap *heap, int index) {
  while (1) {
    int left = index * 2 + 1;
    int right = index * 2 + 2;
    int largest = index;

    if (left < heap->size &&
        heap->entries[left]->value > heap->entries[largest]->value)
      largest = left;

    if (right < heap->size &&
        heap->entries[right]->value > heap->entries[largest]->value)
      largest = right;

    if (largest == index)
      break;

    swap(&heap->entries[index], &heap->entries[largest]);
    index = largest;
  }
}

Heap *heap_create(int capacity) {
  if (capacity <= 0)
    return NULL;

  Heap *heap = malloc(sizeof(Heap));

  if (!heap)
    return NULL;

  heap->entries = malloc(capacity * sizeof(Entry *));

  if (!heap->entries) {
    free(heap);
    return NULL;
  }

  heap->size = 0;
  heap->capacity = capacity;

  return heap;
}

void heap_free(Heap *heap) {
  if (!heap)
    return;

  free(heap->entries);
  free(heap);
}

int heap_insert(Heap *heap, Entry *entry) {
  if (!heap || !entry)
    return 0;

  if (heap->size == heap->capacity)
    return 0;

  heap->entries[heap->size] = entry;

  sift_up(heap, heap->size);

  heap->size++;

  return 1;
}

Entry *heap_peek(Heap *heap) {
  if (!heap || heap->size == 0)
    return NULL;

  return heap->entries[0];
}

Entry *heap_extract_max(Heap *heap) {
  if (!heap || heap->size == 0)
    return NULL;

  Entry *max = heap->entries[0];

  heap->size--;

  if (heap->size > 0) {
    heap->entries[0] = heap->entries[heap->size];
    sift_down(heap, 0);
  }

  return max;
}

int heap_size(Heap *heap) {
  if (!heap)
    return 0;

  return heap->size;
}
