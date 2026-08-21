#include <stdlib.h>

#include "buffer_pool.h"

typedef struct {
  Page page;
  int page_id;
  int valid;
  int dirty;
} Frame;

struct BufferPool {
  PageManager *manager;
  Frame frames[BUFFER_POOL_SIZE];
};

BufferPool *buffer_pool_create(PageManager *manager) {
  if (!manager)
    return NULL;

  BufferPool *pool = malloc(sizeof(BufferPool));

  if (!pool)
    return NULL;

  pool->manager = manager;

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
    pool->frames[i].page_id = -1;
    pool->frames[i].valid = 0;
    pool->frames[i].dirty = 0;
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

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
    Frame *frame = &pool->frames[i];

    if (frame->valid && frame->page_id == page_id)
      return &frame->page;
  }

  for (int i = 0; i < BUFFER_POOL_SIZE; i++) {

    Frame *frame = &pool->frames[i];

    if (!frame->valid) {
      if (!page_manager_read(pool->manager, page_id, &frame->page))
        return NULL;

      frame->page_id = page_id;
      frame->valid = 1;
      frame->dirty = 0;

      return &frame->page;
    }
  }

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
