#include <stdio.h>

#include "db.h"
#include "http_server.h"
#include "worker_pool.h"

int main(void) {

  DB *db = db_create();

  if (!db) {
    fprintf(stderr, "failed to create database\n");
    return 1;
  }

  WorkerPool *pool = worker_pool_create(db, 4, 10);

  if (!pool) {
    fprintf(stderr, "failed to create pool\n");
    db_free(db);
    return 1;
  }

  HTTPServer *server = http_server_create(8080, pool);

  if (!server) {
    fprintf(stderr, "failed to create server\n");
    worker_pool_free(pool);
    db_free(db);
    return 1;
  }

  if (!http_server_start(server)) {
    fprintf(stderr, "failed to start HTTP server\n");
    http_server_free(server);
    worker_pool_free(pool);
    db_free(db);
    return 1;
  }

  printf("9oumamaDB listening on port 8080\n");

  http_server_run(server);

  http_server_free(server);
  worker_pool_free(pool);
  db_free(db);

  return 0;
}
