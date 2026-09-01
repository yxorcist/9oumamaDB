#ifndef ENTRY_H
#define ENTRY_H

#define NICKNAME_MAX 32

typedef struct Entry {
  int key;
  int value;
  char nickname[NICKNAME_MAX];
} Entry;

#endif
