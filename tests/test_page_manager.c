#include <stdio.h>
#include <string.h>

#include "page_manager.h"

int main(void) {
  const char *filename = "test_pages.db";
  PageManager *manager = page_manager_create(filename);

  if (!manager) {
    printf("failed to create page manager\n");
    return 1;
  }

  Page page0;
  Page page1;

  memset(&page0, 0, sizeof(Page));
  memset(&page1, 0, sizeof(Page));

  strcpy((char *)page0.data, "PAGE ZERO");
  strcpy((char *)page1.data, "PAGE ONE");

  page_manager_write(manager, 0, &page0);
  page_manager_write(manager, 1, &page1);

  memset(&page0, 0, sizeof(Page));
  memset(&page1, 0, sizeof(Page));

  page_manager_read(manager, 0, &page0);
  page_manager_read(manager, 1, &page1);

  printf("%s\n", page0.data);
  printf("%s\n", page1.data);

  page_manager_free(manager);
  remove(filename);

  return 0;
}
