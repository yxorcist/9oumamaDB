#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "page_manager.h"

struct PageManager {
  FILE *file;
};

PageManager *page_manager_create(const char *filename) {

  if (!filename)
    return NULL;

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

  if (manager->file)
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

  if (fflush(manager->file) != 0)
    return 0;

  return 1;
}

int page_manager_read(PageManager *manager, int page_id, Page *page) {
  if (!manager || !page || page_id < 0)
    return 0;

  long offset = (long)page_id * PAGE_SIZE;
  if (fseek(manager->file, offset, SEEK_SET) != 0)
    return 0;

  size_t bytes_read = fread(page->data, 1, PAGE_SIZE, manager->file);

  if (bytes_read != PAGE_SIZE)
    return 0;

  return 1;
}

int page_manager_page_exists(PageManager *manager, int page_id) {
  if (!manager || page_id < 0)
    return 0;

  if (fseek(manager->file, 0, SEEK_END) != 0)
    return 0;

  long size = ftell(manager->file);

  if (size < 0)
    return 0;

  long required_size = ((long)page_id + 1) * PAGE_SIZE;

  return size >= required_size;
}
