#include <stdlib.h>

#include "buffer_pool.h"

typedef struct {
  Page page;
  int page_id;
  int valid;
  int dirty;
  unsigned long last_used;
} Frame;

struct BufferPool {
  PageManager *manager;
  Frame frames[BUFFER_POOL_SIZE];
  unsigned long clock;
};

BufferPool *buffer_pool_create(PageManager *manager) {
  if (!manager)
    return NULL;

  BufferPool *pool = malloc(sizeof(BufferPool));

  if (!pool)
    return NULL;

  pool->manager = manager;
  pool->clock = 0;

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
    pool->frames[i].page_id = -1;
    pool->frames[i].valid = 0;
    pool->frames[i].dirty = 0;
    pool->frames[i].last_used = 0;
  }

  return pool;
}

void buffer_pool_free(BufferPool *pool) {
  if (!pool)
    return;

  buffer_pool_flush(pool);

  free(pool);
}

Page *buffer_pool_get(BufferPool *pool, int page_id) {
  if (!pool || page_id < 0)
    return NULL;

  /*
   * 1. check weather pgae is already cached
   */
  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
    Frame *frame = &pool->frames[i];

    if (frame->valid && frame->page_id == page_id) {
      frame->last_used = pool->clock++;
      return &frame->page;
    }
  }

  /*
   * 2. look for an unused frame
   */
  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
    Frame *frame = &pool->frames[i];

    if (!frame->valid) {
      if (!page_manager_read(pool->manager, page_id, &frame->page))
        return NULL;

      frame->page_id = page_id;
      frame->valid = 1;
      frame->dirty = 0;
      frame->last_used = pool->clock++;

      return &frame->page;
    }
  }

  /*
   * 3. Pool is full
   * Find the least recently used frame
   */
  int lru_index = 0;

  for (int i = 1; i < BUFFER_POOL_SIZE; i++) {
    if (pool->frames[i].last_used < pool->frames[lru_index].last_used)
      lru_index = i;
  }

  Frame *frame = &pool->frames[lru_index];

  /*
   * 4. if page was modified
   * write it back before replacing it
   */
  if (frame->dirty) {
    if (!page_manager_write(pool->manager, frame->page_id, &frame->page))
      return NULL;
  }

  /*
   * 5. if page was modified
   * write it back before replacing it
   */
  if (!page_manager_read(pool->manager, page_id, &frame->page))
    return NULL;

  frame->page_id = page_id;
  frame->valid = 1;
  frame->dirty = 0;
  frame->last_used = pool->clock++;

  return NULL;
}

void buffer_pool_mark_dirty(BufferPool *pool, int page_id) {

  if (!pool)
    return;

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {

    Frame *frame = &pool->frames[i];

    if (frame->valid && frame->page_id == page_id) {
      frame->dirty = 1;
      return;
    }
  }
}

int buffer_pool_flush(BufferPool *pool) {
  if (!pool)
    return 0;

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {

    Frame *frame = &pool->frames[i];

    if (!frame->valid || !frame->dirty)
      continue;

    if (!page_manager_write(pool->manager, frame->page_id, &frame->page))
      return 0;

    frame->dirty = 0;
  }

  return 1;
}
