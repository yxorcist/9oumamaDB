#include <pthread.h>
#include <stdlib.h>

#include "request_queue.h"

struct RequestQueue {
  Request **requests;

  int capacity;
  int count;

  int head;
  int tail;

  int shutdown;

  pthread_mutex_t mutex;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
};

RequestQueue *request_queue_create(int capacity) {
  if (capacity <= 0)
    return NULL;

  RequestQueue *queue = malloc(sizeof(RequestQueue));
  if (!queue)
    return NULL;

  queue->requests = malloc(capacity * sizeof(Request *));

  if (!queue->requests) {
    free(queue);
    return NULL;
  }

  queue->capacity = capacity;
  queue->count = 0;
  queue->head = 0;
  queue->tail = 0;
  queue->shutdown = 0;

  if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
    free(queue->requests);
    free(queue);
    return NULL;
  }

  if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
    pthread_mutex_destroy(&queue->mutex);
    free(queue->requests);
    free(queue);
    return NULL;
  }

  if (pthread_cond_init(&queue->not_full, NULL) != 0) {
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->requests);
    free(queue);
    return NULL;
  }

  return queue;
}

void request_queue_free(RequestQueue *queue) {
  if (!queue)
    return;

  pthread_cond_destroy(&queue->not_empty);
  pthread_cond_destroy(&queue->not_full);
  pthread_mutex_destroy(&queue->mutex);

  free(queue->requests);
  free(queue);
}

int request_queue_push(RequestQueue *queue, Request *request) {
  if (!queue || !request)
    return 0;

  pthread_mutex_lock(&queue->mutex);

  while (queue->count == queue->capacity && !queue->shutdown)
    pthread_cond_wait(&queue->not_full, &queue->mutex);

  if (queue->shutdown) {
    pthread_mutex_unlock(&queue->mutex);
    return 0;
  }

  queue->requests[queue->tail] = request;

  queue->tail = (queue->tail + 1) % queue->capacity;
  queue->count++;

  pthread_cond_signal(&queue->not_empty);

  pthread_mutex_unlock(&queue->mutex);

  return 1;
}

int request_queue_pop(RequestQueue *queue, Request **request) {
  if (!queue || !request)
    return 0;

  pthread_mutex_lock(&queue->mutex);

  while (queue->count == 0 && !queue->shutdown)
    pthread_cond_wait(&queue->not_empty, &queue->mutex);

  if (queue->count == 0 && queue->shutdown) {
    pthread_mutex_unlock(&queue->mutex);
    return 0;
  }

  *request = queue->requests[queue->head];

  queue->head = (queue->head + 1) % queue->capacity;
  queue->count--;

  pthread_cond_signal(&queue->not_full);

  pthread_mutex_unlock(&queue->mutex);

  return 1;
}

int request_queue_shutdown(RequestQueue *queue) {
  if (!queue)
    return 0;

  pthread_mutex_lock(&queue->mutex);

  queue->shutdown = 1;

  pthread_cond_broadcast(&queue->not_empty);
  pthread_cond_broadcast(&queue->not_full);

  pthread_mutex_unlock(&queue->mutex);

  return 1;
}

int request_init(Request *request, CommandType type, int key, int value) {
  if (!request)
    return 0;

  request->type = type;
  request->key = key;
  request->value = value;

  request->result = 0;
  request->found = 0;
  request->done = 0;
  request->entries = NULL;
  request->entry_count = 0;

  if (pthread_mutex_init(&request->mutex, NULL) != 0)
    return 0;

  if (pthread_cond_init(&request->completed, NULL) != 0) {
    pthread_mutex_destroy(&request->mutex);
    return 0;
  }

  request->entries = malloc(REQUEST_RESULT_CAPACITY * sizeof(Entry));

  if (!request->entries) {
    pthread_cond_destroy(&request->completed);
    pthread_mutex_destroy(&request->mutex);
    return 0;
  }

  return 1;
}

void request_destroy(Request *request) {
  if (!request)
    return;

  pthread_cond_destroy(&request->completed);
  pthread_mutex_destroy(&request->mutex);

  free(request->entries);
  request->entries = NULL;
}

void request_wait(Request *request) {
  if (!request)
    return;

  pthread_mutex_lock(&request->mutex);

  while (!request->done)
    pthread_cond_wait(&request->completed, &request->mutex);

  pthread_mutex_unlock(&request->mutex);
}

void request_complete(Request *request) {
  if (!request)
    return;

  pthread_mutex_lock(&request->mutex);

  request->done = 1;

  pthread_cond_signal(&request->completed);
  pthread_mutex_unlock(&request->mutex);
}
