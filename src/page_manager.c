#include <stdio.h>
#include <stdlib.h>

#include "page_manager.h"

struct PageManager {
  FILE *file;
};

PageManager *page_manager_create(const char *filename) {
  PageManager *manager = malloc(sizeof(PageManager));

  if (!manager)
    return NULL;

  manager->file = fopen(filename, "r+b");

  if (!manager->file) {
    manager->file = fopen(filename, "w+b");
    if (!manager->file) {
      free(manager);
      return NULL;
    }
  }

  return manager;
}

void page_manager_free(PageManager *manager) {
  if (!manager)
    return;

  fclose(manager->file);
  free(manager);
}

int page_manager_write(PageManager *manager, int page_id, Page *page) {
  if (!manager || !page || page_id < 0)
    return 0;

  long offset = (long)page_id * PAGE_SIZE;

  if (fseek(manager->file, offset, SEEK_SET) != 0)
    return 0;

  if (fwrite(page->data, 1, PAGE_SIZE, manager->file) != PAGE_SIZE)
    return 0;

  fflush(manager->file);

  return 1;
}

int page_manager_read(PageManager *manager, int page_id, Page *page) {
  if (!manager || !page || page_id < 0)
    return 0;

  long offset = (long)page_id * PAGE_SIZE;

  if (fseek(manager->file, offset, SEEK_SET) != 0)
    return 0;

  if (fread(page->data, 1, PAGE_SIZE, manager->file) != PAGE_SIZE)
    return 0;

  return 1;
}
