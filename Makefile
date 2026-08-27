CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -pthread -g

SRC = $(shell find src -name '*.c')
OUT = build/9oumamaDB

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

test:
	gcc $(CFLAGS) \
		src/page_manager.c \
		tests/test_page_manager.c \
		-o test_page_manager

	gcc $(CFLAGS) \
		src/buffer_pool.c \
		src/page_manager.c \
		tests/test_buffer_pool.c \
		-o test_buffer_pool

	gcc $(CFLAGS) \
    src/hash_table.c \
    src/bst.c \
    src/heap.c \
    src/db.c \
    src/storage.c \
    src/buffer_pool.c \
    src/page_manager.c \
		src/request_queue.c \
    tests/test_db.c \
    -o test_db

	gcc $(CFLAGS) \
		src/storage.c \
		src/buffer_pool.c \
		src/page_manager.c \
		tests/test_storage.c \
		-o test_storage

	gcc $(CFLAGS) \
    src/heap.c \
    tests/test_heap.c \
    -o test_heap

	gcc $(CFLAGS) \
    src/parser.c \
    tests/test_parser.c \
    -o test_parser

	gcc $(CFLAGS) \
    src/hash_table.c \
    src/bst.c \
    src/heap.c \
    src/db.c \
    src/storage.c \
    src/buffer_pool.c \
    src/page_manager.c \
		src/request_queue.c \
    tests/test_concurrency.c \
    -o test_concurrency

	gcc $(CFLAGS) -pthread \
    src/request_queue.c \
    tests/test_request_queue.c \
    -o test_request_queue

	gcc $(CFLAGS) -pthread \
    src/request_queue.c \
    src/worker_pool.c \
    src/db.c \
    src/storage.c \
    src/page_manager.c \
		src/buffer_pool.c \
    src/hash_table.c \
    src/bst.c \
    src/heap.c \
    tests/test_worker_pool.c \
    -o test_worker_pool

	gcc $(CFLAGS) \
    src/http_server.c \
    tests/test_http_server.c \
    -o test_http_server

	gcc -Wall -Wextra -std=c11 -Iinclude -g \
    src/http_connection.c \
    tests/test_http_connection.c \
    -o test_http_connection

	gcc -Wall -Wextra -std=c11 -Iinclude -g \
    src/http_request.c \
    tests/test_http_request.c \
    -o test_http_request

	gcc -Wall -Wextra -std=c11 -Iinclude -pthread -g \
    src/http_request.c \
    src/http_dispatch.c \
    src/request_queue.c \
    tests/test_http_dispatch.c \
    -o test_http_dispatch

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

clean:
	rm -rf build
	rm -f test_page_manager test_buffer_pool test_storage \
	test_db test_heap test_parser test_concurrency *.db \
	test_request_queue test_worker_pool test_http_server \
	test_http_connection test_http_request test_http_dispatch

