#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer_pool.h"
#include "page_manager.h"

#define TEST_FILE "test_buffer_pool.db"

static void test_lru_eviction(void) {
  remove(TEST_FILE);

  PageManager *manager = page_manager_create(TEST_FILE);
  assert(manager != NULL);

  BufferPool *pool = buffer_pool_create(manager);
  assert(pool != NULL);

  /*
   * The pool has 16 frames.
   * Create 20 pages so eviction is guaranteed.
   */
  for (int page_id = 0; page_id < 20; page_id++) {
    assert(buffer_pool_new_page(pool, page_id));

    Page *page = buffer_pool_get(pool, page_id);
    assert(page != NULL);

    memset(page->data, 0, PAGE_SIZE);

    memcpy(page->data, &page_id, sizeof(page_id));

    buffer_pool_mark_dirty(pool, page_id);
  }

  /*
   * Flush everything currently cached.
   */
  assert(buffer_pool_flush(pool));

  /*
   * Every page must exist on disk, including pages
   * that were evicted earlier.
   */
  for (int page_id = 0; page_id < 20; page_id++) {
    Page page;

    assert(page_manager_read(manager, page_id, &page));

    int stored_id;

    memcpy(&stored_id, page.data, sizeof(stored_id));

    assert(stored_id == page_id);
  }

  buffer_pool_free(pool);
  page_manager_free(manager);

  remove(TEST_FILE);

  printf("PASS: LRU eviction\n");
}

int main(void) {
  test_lru_eviction();

  printf("\nAll buffer pool tests passed.\n");

  return 0;
}
