#ifndef PAGE_H
#define PAGE_H

#define PAGE_SIZE 4096

#define PAGE_TYPE_FREE 0
#define PAGE_TYPE_METADATA 1
#define PAGE_TYPE_DATA 2

typedef struct {
  unsigned char data[PAGE_SIZE];
} Page;

typedef struct {
  int type;
} PageHeader;

#endif
