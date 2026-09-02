#ifndef DB_H
#define DB_H

#include "entry.h"
#include "request_queue.h"

typedef struct DB DB;

DB *db_create(void);
void db_free(DB *db);

int db_insert(DB *db, int key, int value, const char *nickname);
int db_get(DB *db, int key, int *found);
int db_delete(DB *db, int key);
int db_update(DB *db, int key, int value);

int db_get_entry(DB *db, int key, Entry *result);

int db_topk(DB *db, int k, Entry *results);

int db_clear(DB *db);

int db_range(DB *db, int a, int b, Entry *results, int capacity);
int db_count(DB *db);

int db_save(DB *db);
int db_load(DB *db);

int db_begin(DB *db);
int db_commit(DB *db);
int db_rollback(DB *db);

int db_in_transaction(DB *db);

int db_execute_request(DB *db, Request *request);

#endif
