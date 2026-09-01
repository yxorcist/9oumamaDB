#include <assert.h>
#include <stdio.h>

#include "entry.h"
#include "heap.h"

int main(void) {
  Entry a = {1, 40, ""};
  Entry b = {2, 90, ""};
  Entry c = {3, 15, ""};
  Entry d = {4, 70, ""};
  Entry e = {5, 100, ""};

  Heap *heap = heap_create(5);
  assert(heap != NULL);

  assert(heap_insert(heap, &a));
  assert(heap_insert(heap, &b));
  assert(heap_insert(heap, &c));
  assert(heap_insert(heap, &d));
  assert(heap_insert(heap, &e));

  assert(heap_size(heap) == 5);

  /* Remove the maximum */
  assert(heap_peek(heap) == &e);
  assert(heap_remove(heap, &e));
  assert(heap_size(heap) == 4);
  assert(heap_peek(heap) == &b);

  /* Remove an element from the middle */
  assert(heap_remove(heap, &d));
  assert(heap_size(heap) == 3);
  assert(heap_peek(heap) == &b);

  /* Verify remaining ordering */
  assert(heap_extract_max(heap) == &b);
  assert(heap_extract_max(heap) == &a);
  assert(heap_extract_max(heap) == &c);

  assert(heap_size(heap) == 0);
  assert(heap_extract_max(heap) == NULL);
  assert(heap_peek(heap) == NULL);

  /* Removing an entry that isn't present must fail */
  assert(!heap_remove(heap, &e));

  heap_free(heap);

  printf("All heap tests passed.\n");

  return 0;
}
