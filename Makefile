CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -pthread -g

SRC = $(shell find src -name '*.c')
OUT = build/9oumamaDB

DB_SRC = \
	src/hash_table.c \
	src/bst.c \
	src/heap.c \
	src/db.c \
	src/storage.c \
	src/buffer_pool.c \
	src/page_manager.c

CONCURRENCY_SRC = \
	src/request_queue.c \
	src/worker_pool.c

HTTP_SRC = \
	src/http_server.c \
	src/http_connection.c \
	src/http_request.c \
	src/http_response.c \
	src/http_dispatch.c

TESTS = \
	test_page_manager \
	test_buffer_pool \
	test_storage \
	test_db \
	test_heap \
	test_parser \
	test_concurrency \
	test_request_queue \
	test_worker_pool \
	test_http_server \
	test_http_connection \
	test_http_request \
	test_http_dispatch \
	test_http_response

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

test: $(TESTS)
	./test_page_manager
	./test_buffer_pool
	./test_storage
	./test_db
	./test_heap
	./test_parser
	./test_concurrency
	./test_request_queue
	./test_worker_pool
	./test_http_server
	./test_http_connection
	./test_http_request
	./test_http_dispatch
	./test_http_response

test_page_manager: src/page_manager.c tests/test_page_manager.c
	$(CC) $(CFLAGS) $^ -o $@

test_buffer_pool: src/buffer_pool.c src/page_manager.c tests/test_buffer_pool.c
	$(CC) $(CFLAGS) $^ -o $@

test_storage: src/storage.c src/buffer_pool.c src/page_manager.c tests/test_storage.c
	$(CC) $(CFLAGS) $^ -o $@

test_db: $(DB_SRC) tests/test_db.c
	$(CC) $(CFLAGS) $^ -o $@

test_heap: src/heap.c tests/test_heap.c
	$(CC) $(CFLAGS) $^ -o $@

test_parser: src/parser.c tests/test_parser.c
	$(CC) $(CFLAGS) $^ -o $@

test_concurrency: $(DB_SRC) $(CONCURRENCY_SRC) tests/test_concurrency.c
	$(CC) $(CFLAGS) $^ -o $@

test_request_queue: src/request_queue.c tests/test_request_queue.c
	$(CC) $(CFLAGS) $^ -o $@

test_worker_pool: $(DB_SRC) $(CONCURRENCY_SRC) tests/test_worker_pool.c
	$(CC) $(CFLAGS) $^ -o $@

test_http_server: $(HTTP_SRC) $(CONCURRENCY_SRC) $(DB_SRC) tests/test_http_server.c
	$(CC) $(CFLAGS) $^ -o $@

test_http_connection: $(HTTP_SRC) $(CONCURRENCY_SRC) $(DB_SRC) tests/test_http_connection.c
	$(CC) $(CFLAGS) $^ -o $@

test_http_request: src/http_request.c tests/test_http_request.c
	$(CC) $(CFLAGS) $^ -o $@

test_http_dispatch: \
	src/http_request.c \
	src/http_dispatch.c \
	src/request_queue.c \
	tests/test_http_dispatch.c
	$(CC) $(CFLAGS) $^ -o $@

test_http_response: src/http_response.c tests/test_http_response.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
	rm -f $(TESTS) *.db
