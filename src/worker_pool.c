#include <pthread.h>
#include <stdlib.h>

#include "worker_pool.h"

struct WorkerPool {
  DB *db;
  RequestQueue *queue;
  pthread_t *workers;
  int worker_count;
};

static void *worker_main(void *arg) {
  WorkerPool *pool = arg;

  Request *request;

  while (request_queue_pop(pool->queue, &request)) {
    db_execute_request(pool->db, request);
    request_complete(request);
  }

  return NULL;
}

WorkerPool *worker_pool_create(DB *db, int worker_count, int queue_capacity) {
  if (worker_count <= 0 || queue_capacity <= 0 || !db)
    return NULL;

  WorkerPool *pool = malloc(sizeof(WorkerPool));

  if (!pool)
    return NULL;

  pool->queue = request_queue_create(queue_capacity);

  pool->db = db;

  if (!pool->queue) {
    free(pool);
    return NULL;
  }

  pool->workers = malloc(worker_count * sizeof(pthread_t));

  if (!pool->workers) {
    request_queue_free(pool->queue);
    free(pool);
    return NULL;
  }

  pool->worker_count = worker_count;

  for (int i = 0; i < worker_count; i++) {
    if (pthread_create(&pool->workers[i], NULL, worker_main, pool) != 0) {
      request_queue_shutdown(pool->queue);

      for (int j = 0; j < i; j++)
        pthread_join(pool->workers[j], NULL);

      free(pool->workers);
      request_queue_free(pool->queue);
      free(pool);

      return NULL;
    }
  }

  return pool;
}

int worker_pool_submit(WorkerPool *pool, Request *request) {
  if (!pool || !request)
    return 0;

  return request_queue_push(pool->queue, request);
}

void worker_pool_free(WorkerPool *pool) {
  if (!pool)
    return;

  request_queue_shutdown(pool->queue);

  for (int i = 0; i < pool->worker_count; i++)
    pthread_join(pool->workers[i], NULL);

  free(pool->workers);
  request_queue_free(pool->queue);
  free(pool);
}
