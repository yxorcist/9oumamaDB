#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <pthread.h>

#include "entry.h"
#include "parser.h"

#define REQUEST_RESULT_CAPACITY 100

typedef struct {
  CommandType type;
  int key;
  int value;

  int a;
  int b;
  int k;

  int result;
  int found;

  Entry *entries;
  int entry_count;

  pthread_mutex_t mutex;
  pthread_cond_t completed;
  int done;
} Request;

typedef struct RequestQueue RequestQueue;

RequestQueue *request_queue_create(int capacity);
void request_queue_free(RequestQueue *queue);

int request_queue_push(RequestQueue *queue, Request *request);
int request_queue_pop(RequestQueue *queue, Request **request);

int request_queue_shutdown(RequestQueue *queue);

int request_init(Request *request, CommandType type, int key, int value);

void request_destroy(Request *request);
void request_wait(Request *request);
void request_complete(Request *request);

#endif
