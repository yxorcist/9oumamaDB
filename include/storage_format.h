#ifndef STORAGE_FORMAT_H
#define STORAGE_FORMAT_H

#define DB_MAGIC 0x39444231
#define DB_VERSION 1

typedef struct {
  int magic;
  int version;
  int count;
} DatabaseHeader;

#endif
