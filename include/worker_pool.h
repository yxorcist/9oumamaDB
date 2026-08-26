#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "db.h"
#include "request_queue.h"

typedef struct WorkerPool WorkerPool;

WorkerPool *worker_pool_create(DB *db, int worker_count, int queue_capacity);
void worker_pool_free(WorkerPool *pool);

int worker_pool_submit(WorkerPool *pool, Request *request);

#endif
