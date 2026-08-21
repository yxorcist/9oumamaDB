#include <stdio.h>
#include <string.h>

#include "buffer_pool.h"
#include "page_manager.h"

/*

gcc -Wall -Wextra -Iinclude \
src/page_manager.c          \
src/buffer_pool.c           \
tests/test_buffer_pool.c    \
-o test_buffer_pool         \
&& ./test_buffer_pool

*/

int main(void) {
  const char *filename = "test_buffer.db";

  PageManager *manager = page_manager_create(filename);

  if (!manager) {
    printf("failed to create page manager\n");
    return 1;
  }

  /*
   * Create some pages directly on disk
   */
  Page page;

  memset(&page, 0, sizeof(Page));
  strcpy((char *)page.data, "PAGE 0");
  page_manager_write(manager, 0, &page);

  memset(&page, 0, sizeof(Page));
  strcpy((char *)page.data, "PAGE 1");
  page_manager_write(manager, 1, &page);

  memset(&page, 0, sizeof(Page));
  strcpy((char *)page.data, "PAGE 2");
  page_manager_write(manager, 2, &page);

  memset(&page, 0, sizeof(Page));
  strcpy((char *)page.data, "PAGE 3");
  page_manager_write(manager, 3, &page);

  /*
   * Create the buffer pool
   */
  BufferPool *pool = buffer_pool_create(manager);

  if (!pool) {
    printf("failed to create pool\n");
    page_manager_free(manager);
    return 1;
  }

  /*
   * TEST 1: Load Pages
   */
  Page *p0 = buffer_pool_get(pool, 0);
  Page *p1 = buffer_pool_get(pool, 1);
  Page *p2 = buffer_pool_get(pool, 2);

  if (!p0 || !p1 || !p2) {
    printf("failed to load pages\n");
    buffer_pool_free(pool);
    page_manager_free(manager);
    return 1;
  }

  printf("%s\n", p0->data);
  printf("%s\n", p1->data);
  printf("%s\n", p2->data);

  /*
   * TEST2: Cache hit
   */
  Page *cached = buffer_pool_get(pool, 0);

  if (!cached) {
    printf("cache hit failed\n");
    buffer_pool_free(pool);
    page_manager_free(manager);
    return 1;
  }

  printf("cache hit: %s\n", cached->data);

  /*
   * TEST 3: Modify page 0
   */
  strcpy((char *)p0->data, "MODIFIED PAGE 0");
  buffer_pool_mark_dirty(pool, 0);

  /*
   * TEST 4: Make page 0 LRU
   */
  buffer_pool_get(pool, 1);
  buffer_pool_get(pool, 2);

  /*
   * TEST 5: Force eviction
   */
  Page *p3 = buffer_pool_get(pool, 3);

  if (!p3) {
    printf("failed to load page 3\n");
    buffer_pool_free(pool);
    page_manager_free(manager);
    return 1;
  }

  printf("loaded: %s\n", p3->data);

  /*
   * TEST 6: Reload page 0
   */
  Page *reloaded = buffer_pool_get(pool, 0);

  if (!reloaded) {
    printf("failed to reload page 0\n");
    buffer_pool_free(pool);
    page_manager_free(manager);
    return 1;
  }

  printf("reloaded: %s\n", reloaded->data);

  /* CLEANUP */
  buffer_pool_free(pool);
  page_manager_free(manager);

  return 0;
}
