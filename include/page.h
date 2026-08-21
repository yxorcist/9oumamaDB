#ifndef PAGE_H
#define PAGE_H

#define PAGE_SIZE 4096

typedef struct {
  unsigned char data[PAGE_SIZE];
} Page;

#endif
