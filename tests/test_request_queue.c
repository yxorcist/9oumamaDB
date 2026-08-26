#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "request_queue.h"

static void *shutdown_worker(void *arg) {
  RequestQueue *queue = arg;

  Request *request = NULL;

  assert(!request_queue_pop(queue, &request));

  return NULL;
}

static void test_shutdown(void) {
  RequestQueue *queue = request_queue_create(4);
  assert(queue != NULL);

  pthread_t thread;

  assert(pthread_create(&thread, NULL, shutdown_worker, queue) == 0);

  struct timespec ts = {
      .tv_sec = 0,
      .tv_nsec = 100000000,
  };

  nanosleep(&ts, NULL);

  assert(request_queue_shutdown(queue));

  assert(pthread_join(thread, NULL) == 0);

  request_queue_free(queue);

  printf("PASS: request queue shutdown\n");
}

static void *blocked_push_worker(void *arg) {
  RequestQueue *queue = arg;

  Request *request = malloc(sizeof(Request));
  assert(request != NULL);

  assert(request_init(request, CMD_INSERT, 42, 100));

  assert(request_queue_push(queue, request));

  /*
   * Do not destroy the request here.
   * The test thread needs to pop the same request first.
   */

  return NULL;
}

static void test_blocking_push(void) {
  RequestQueue *queue = request_queue_create(1);
  assert(queue != NULL);

  Request first;

  assert(request_init(&first, CMD_INSERT, 1, 10));

  /* Fill the queue. */
  assert(request_queue_push(queue, &first));

  pthread_t thread;

  /* This producer should block because the queue is full. */
  assert(pthread_create(&thread, NULL, blocked_push_worker, queue) == 0);

  struct timespec ts = {
      .tv_sec = 0,
      .tv_nsec = 100000000,
  };

  nanosleep(&ts, NULL);

  /* Free one slot and wake the blocked producer. */
  Request *result = NULL;

  assert(request_queue_pop(queue, &result));

  assert(result == &first);
  assert(result->key == 1);
  assert(result->value == 10);

  assert(pthread_join(thread, NULL) == 0);

  /* Verify the producer's request arrived. */
  result = NULL;

  assert(request_queue_pop(queue, &result));

  assert(result != NULL);
  assert(result->key == 42);
  assert(result->value == 100);

  request_destroy(&first);
  request_destroy(result);

  request_queue_free(queue);

  printf("PASS: blocking push\n");
}

static void *blocked_pop_worker(void *arg) {
  RequestQueue *queue = arg;

  Request *request = NULL;

  assert(request_queue_pop(queue, &request));

  assert(request != NULL);
  assert(request->key == 42);
  assert(request->value == 100);

  return NULL;
}

static void test_blocking_pop(void) {
  RequestQueue *queue = request_queue_create(4);
  assert(queue != NULL);

  pthread_t thread;

  assert(pthread_create(&thread, NULL, blocked_pop_worker, queue) == 0);

  struct timespec ts = {
      .tv_sec = 0,
      .tv_nsec = 100000000,
  };

  nanosleep(&ts, NULL);

  Request request;

  assert(request_init(&request, CMD_INSERT, 42, 100));

  assert(request_queue_push(queue, &request));

  assert(pthread_join(thread, NULL) == 0);

  request_destroy(&request);
  request_queue_free(queue);

  printf("PASS: blocking pop\n");
}

static void test_push_pop(void) {
  RequestQueue *queue = request_queue_create(4);
  assert(queue != NULL);

  Request request;

  assert(request_init(&request, CMD_INSERT, 10, 42));

  assert(request_queue_push(queue, &request));

  Request *result = NULL;

  assert(request_queue_pop(queue, &result));

  assert(result == &request);
  assert(result->key == 10);
  assert(result->value == 42);

  request_destroy(&request);
  request_queue_free(queue);

  printf("PASS: request queue push/pop\n");
}

static void test_fifo(void) {
  RequestQueue *queue = request_queue_create(4);
  assert(queue != NULL);

  Request requests[4];

  for (int i = 0; i < 4; i++) {
    assert(request_init(&requests[i], CMD_INSERT, i, i * 10));

    assert(request_queue_push(queue, &requests[i]));
  }

  for (int i = 0; i < 4; i++) {
    Request *result = NULL;

    assert(request_queue_pop(queue, &result));

    assert(result == &requests[i]);
    assert(result->key == i);
    assert(result->value == i * 10);
  }

  for (int i = 0; i < 4; i++)
    request_destroy(&requests[i]);

  request_queue_free(queue);

  printf("PASS: request queue FIFO\n");
}

int main(void) {
  test_push_pop();
  test_fifo();
  test_blocking_pop();
  test_blocking_push();
  test_shutdown();

  return 0;
}
