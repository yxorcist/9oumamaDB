#include <stdlib.h>
#include <string.h>

#include "buffer_pool.h"
#include "page_manager.h"
#include "storage.h"
#include "storage_format.h"

struct Storage {
  Entry **entries;
  int count;
  int capacity;

  PageManager *page_manager;
  BufferPool *buffer_pool;

  int free_page_head;
  int page_count;
  int active_page_count;
};

static int entries_per_page(void) { return PAGE_SIZE / sizeof(Entry); }

static int page_count_for_entries(int count) {
  int per_page = entries_per_page();

  if (count == 0)
    return 0;

  return (count + per_page - 1) / per_page;
}

static int grow(Storage *storage) {
  int new_capacity = storage->capacity * 2;
  Entry **new_entries =
      realloc(storage->entries, new_capacity * sizeof(Entry *));

  if (!new_entries)
    return 0;
  storage->entries = new_entries;
  storage->capacity = new_capacity;

  return 1;
}

Storage *storage_create(void) {
  Storage *storage = malloc(sizeof(Storage));

  if (!storage)
    return NULL;

  storage->capacity = 16;
  storage->count = 0;

  storage->entries = malloc(storage->capacity * sizeof(Entry *));

  if (!storage->entries) {
    free(storage);
    return NULL;
  }

  storage->page_manager = page_manager_create("database.db");

  storage->free_page_head = NO_FREE_PAGE;
  storage->page_count = 0;
  storage->active_page_count = 0;

  if (!storage->page_manager) {
    free(storage->entries);
    free(storage);
    return NULL;
  }

  storage->buffer_pool = buffer_pool_create(storage->page_manager);

  if (!storage->buffer_pool) {
    page_manager_free(storage->page_manager);
    free(storage->entries);
    free(storage);
    return NULL;
  }

  storage->free_page_head = NO_FREE_PAGE;

  return storage;
}

Entry *storage_create_entry(Storage *storage, int key, int value) {

  if (!storage)
    return NULL;

  if (storage->count == storage->capacity) {
    if (!grow(storage))
      return NULL;
  }

  Entry *entry = malloc(sizeof(Entry));

  if (!entry)
    return NULL;

  entry->key = key;
  entry->value = value;

  storage->entries[storage->count++] = entry;

  return entry;
}

void storage_delete_entry(Storage *storage, Entry *entry) {

  if (!storage || !entry)
    return;

  for (int i = 0; i < storage->count; i++) {

    if (storage->entries[i] != entry)
      continue;

    free(entry);

    for (int j = i; j < storage->count - 1; j++) {
      storage->entries[j] = storage->entries[j + 1];
    }

    storage->count--;
    storage->entries[storage->count] = NULL;

    return;
  }
}

void storage_free(Storage *storage) {
  if (!storage)
    return;

  storage_clear(storage);

  buffer_pool_free(storage->buffer_pool);
  page_manager_free(storage->page_manager);

  free(storage->entries);
  free(storage);
}

int storage_save(Storage *storage) {

  if (!storage)
    return 0;

  /* page 0 contains metadata */
  Page *metadata_page = buffer_pool_get(storage->buffer_pool, 0);

  if (!metadata_page) {

    if (page_manager_page_exists(storage->page_manager, 0))
      return 0;

    if (!buffer_pool_new_page(storage->buffer_pool, 0))
      return 0;

    metadata_page = buffer_pool_get(storage->buffer_pool, 0);

    if (!metadata_page)
      return 0;
  }

  int per_page = entries_per_page();
  int page_count = page_count_for_entries(storage->count);

  if (page_count < storage->active_page_count) {
    for (int page_id = page_count + 1; page_id <= storage->active_page_count;
         page_id++) {
      if (!storage_free_page(storage, page_id))
        return 0;
    }
  }

  storage->active_page_count = page_count;

  if (storage->page_count < page_count)
    storage->page_count = page_count;

  DatabaseHeader header = {.magic = DB_MAGIC,
                           .version = DB_VERSION,
                           .count = storage->count,
                           .free_page_head = storage->free_page_head,
                           .page_count = storage->page_count};

  memset(metadata_page->data, 0, PAGE_SIZE);
  memcpy(metadata_page->data, &header, sizeof(DatabaseHeader));

  buffer_pool_mark_dirty(storage->buffer_pool, 0);

  storage->page_count = page_count;

  /* page 1..N contain entries */
  for (int page_id = 0; page_id < page_count; page_id++) {

    Page *page = buffer_pool_get(storage->buffer_pool, page_id + 1);

    if (!page) {
      if (!buffer_pool_new_page(storage->buffer_pool, page_id + 1))
        return 0;

      page = buffer_pool_get(storage->buffer_pool, page_id + 1);

      if (!page)
        return 0;
    }

    memset(page->data, 0, PAGE_SIZE);

    int start = page_id * per_page;
    int end = start + per_page;

    if (end > storage->count)
      end = storage->count;

    for (int i = start; i < end; i++) {

      int offset = (i - start) * sizeof(Entry);

      memcpy(page->data + offset, storage->entries[i], sizeof(Entry));
    }

    buffer_pool_mark_dirty(storage->buffer_pool, page_id + 1);
  }

  return buffer_pool_flush(storage->buffer_pool);
}

int storage_load(Storage *storage) {

  if (!storage)
    return 0;

  storage_clear(storage);

  if (!page_manager_page_exists(storage->page_manager, 0)) {
    storage->count = 0;
    storage->free_page_head = NO_FREE_PAGE;
    storage->page_count = 0;
    return 1;
  }

  /* read metadata page */
  Page *metadata_page = buffer_pool_get(storage->buffer_pool, 0);

  if (!metadata_page)
    return 0;

  DatabaseHeader header;

  memcpy(&header, metadata_page->data, sizeof(DatabaseHeader));

  /* validate database file format */
  if (header.magic != DB_MAGIC)
    return 0;

  if (header.version != DB_VERSION)
    return 0;

  int entry_count = header.count;

  if (entry_count < 0)
    return 0;

  if (header.page_count < 0)
    return 0;

  storage->free_page_head = header.free_page_head;
  storage->page_count = header.page_count;

  int required_pages = page_count_for_entries(entry_count);

  if (storage->page_count < required_pages)
    return 0;

  storage->active_page_count = required_pages;

  if (header.page_count < required_pages)
    return 0;

  storage->free_page_head = header.free_page_head;
  storage->page_count = header.page_count;

  /* verify if storage has enough capacity */
  while (storage->capacity < entry_count) {
    if (!grow(storage))
      return 0;
  }

  int per_page = entries_per_page();
  int page_count = page_count_for_entries(entry_count);

  /* load entries */
  for (int page_id = 0; page_id < page_count; page_id++) {

    Page *page = buffer_pool_get(storage->buffer_pool, page_id + 1);

    if (!page)
      goto load_failure;

    int start = page_id * per_page;
    int end = start + per_page;

    if (end > entry_count)
      end = entry_count;

    for (int i = start; i < end; i++) {
      int offset = (i - start) * sizeof(Entry);
      Entry *entry = malloc(sizeof(Entry));

      if (!entry)
        goto load_failure;

      memcpy(entry, page->data + offset, sizeof(Entry));

      storage->entries[i] = entry;
    }
  }

  storage->count = entry_count;

  return 1;

load_failure:
  /* Free entries that were successfully loaded */
  for (int i = 0; i < entry_count; i++) {
    free(storage->entries[i]);
    storage->entries[i] = NULL;
  }
  storage->count = 0;

  return 0;
}

void storage_clear(Storage *storage) {

  if (!storage)
    return;

  for (int i = 0; i < storage->count; i++)
    free(storage->entries[i]);

  storage->count = 0;

  if (storage->entries && storage->capacity > 0)
    storage->entries[0] = NULL;
}

int storage_count(Storage *storage) {
  if (!storage)
    return 0;

  return storage->count;
}

Entry *storage_get_entry(Storage *storage, int index) {
  if (!storage)
    return NULL;

  if (index < 0 || index >= storage->count)
    return NULL;

  return storage->entries[index];
}

int storage_allocate_page(Storage *storage) {
  if (!storage)
    return -1;

  /* Reuse a page from the free-page list */
  if (storage->free_page_head != NO_FREE_PAGE) {
    int page_id = storage->free_page_head;

    Page *page = buffer_pool_get(storage->buffer_pool, page_id);

    if (!page)
      return -1;

    FreePage free_page;

    memcpy(&free_page, page->data, sizeof(FreePage));

    storage->free_page_head = free_page.next_free_page;

    return page_id;
  }

  /* No free pages available */
  int page_id = storage->page_count + 1;

  if (!buffer_pool_new_page(storage->buffer_pool, page_id))
    return -1;

  storage->page_count++;

  return page_id;
}

int storage_free_page(Storage *storage, int page_id) {
  if (!storage || page_id <= 0)
    return 0;

  Page *page = buffer_pool_get(storage->buffer_pool, page_id);

  if (!page)
    return 0;

  FreePage free_page = {.next_free_page = storage->free_page_head};

  memset(page->data, 0, PAGE_SIZE);
  memcpy(page->data, &free_page, sizeof(FreePage));

  storage->free_page_head = page_id;

  buffer_pool_mark_dirty(storage->buffer_pool, page_id);

  return 1;
}

void storage_discard_pages(Storage *storage) {
  if (!storage)
    return;

  buffer_pool_discard(storage->buffer_pool);
}

void storage_set_writes_enabled(Storage *storage, int enabled) {
  if (!storage)
    return;

  buffer_pool_set_writes_enabled(storage->buffer_pool, enabled);
}
