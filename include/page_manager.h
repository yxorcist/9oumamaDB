#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "page.h"

typedef struct PageManager PageManager;

PageManager *page_manager_create(const char *filename);
void page_manager_free(PageManager *manager);

int page_manager_write(PageManager *manager, int page_id, Page *page);
int page_manager_read(PageManager *manager, int page_id, Page *page);

#endif
