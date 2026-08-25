#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "page.h"
#include "page_manager.h"

#define BUFFER_POOL_SIZE 16

typedef struct BufferPool BufferPool;

BufferPool *buffer_pool_create(PageManager *manager);
void buffer_pool_free(BufferPool *pool);

Page *buffer_pool_get(BufferPool *pool, int page_id);
int buffer_pool_new_page(BufferPool *pool, int page_id);
int buffer_pool_flush(BufferPool *pool);

void buffer_pool_mark_dirty(BufferPool *pool, int page_id);
void buffer_pool_discard(BufferPool *pool);
void buffer_pool_set_writes_enabled(BufferPool *pool, int enabled);

#endif
