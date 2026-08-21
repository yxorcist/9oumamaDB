#include <stdio.h>
#include <string.h>

#include "buffer_pool.h"
#include "page_manager.h"

/*

gcc -Wall -Wextra -Iinclude \
src/page_manager.c          \
src/buffer_pool.c           \
tests/test_buffer_pool.c    \
-o test_buffer_pool

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
   * Fill all three frames
   */
  Page *p0 = buffer_pool_get(pool, 0);
  Page *p1 = buffer_pool_get(pool, 1);
  Page *p2 = buffer_pool_get(pool, 2);

  printf("%s\n", p0->data);
  printf("%s\n", p1->data);
  printf("%s\n", p2->data);

  /*
   * Access page 0 again
   * this makes page 1 the least recently used
   */
  buffer_pool_get(pool, 0);

  /*
   * request page 3
   * page 1 should be evicted
   */
  Page *p3 = buffer_pool_get(pool, 3);

  printf("%s\n", p3->data);

  buffer_pool_free(pool);
  page_manager_free(manager);

  return 0;
}
