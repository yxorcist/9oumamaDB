#ifndef STORAGE_FORMAT_H
#define STORAGE_FORMAT_H

#define DB_MAGIC 0x39444231
#define DB_VERSION 2
#define NO_FREE_PAGE -1

typedef struct {
  int magic;
  int version;
  int count;
  int free_page_head;
  int page_count;
} DatabaseHeader;

typedef struct {
  int next_free_page;
} FreePage;

#endif
