#ifndef DB_H
#define DB_H

typedef struct DB DB;

DB *db_create(void);
void db_free(DB *db);

void db_insert(DB *db, int key, int value);
int db_get(DB *db, int key, int *found);
void db_delete(DB *db, int key);

void db_range(DB *db, int a, int b);

#endif
