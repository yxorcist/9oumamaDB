#ifndef DB_H
#define DB_H

#include "entry.h"

typedef struct DB DB;

DB *db_create(void);
void db_free(DB *db);

void db_insert(DB *db, int key, int value);
int db_get(DB *db, int key, int *found);
void db_delete(DB *db, int key);
void db_update(DB *db, int key, int value);

int db_topk(DB *db, int k, Entry **results);

void db_clear(DB *db);

void db_range(DB *db, int a, int b);
int db_count(DB *db);

int db_save(DB *db);
int db_load(DB *db);

int db_begin(DB *db);
int db_commit(DB *db);
int db_rollback(DB *db);

#endif
