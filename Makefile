CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -pthread -g

SRC = $(filter-out src/main.c,$(wildcard src/*.c))

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
	test_http_concurrency \
	test_http_request \
	test_http_dispatch \
	test_http_response

TEST_DIR = build/tests

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) src/main.c -o build/9oumamaDB

run: all
	./build/9oumamaDB

test:
	mkdir -p $(TEST_DIR)

	$(CC) $(CFLAGS) $(SRC) tests/test_page_manager.c -o $(TEST_DIR)/test_page_manager
	$(CC) $(CFLAGS) $(SRC) tests/test_buffer_pool.c -o $(TEST_DIR)/test_buffer_pool
	$(CC) $(CFLAGS) $(SRC) tests/test_storage.c -o $(TEST_DIR)/test_storage
	$(CC) $(CFLAGS) $(SRC) tests/test_db.c -o $(TEST_DIR)/test_db
	$(CC) $(CFLAGS) $(SRC) tests/test_heap.c -o $(TEST_DIR)/test_heap
	$(CC) $(CFLAGS) $(SRC) tests/test_parser.c -o $(TEST_DIR)/test_parser
	$(CC) $(CFLAGS) $(SRC) tests/test_concurrency.c -o $(TEST_DIR)/test_concurrency
	$(CC) $(CFLAGS) $(SRC) tests/test_request_queue.c -o $(TEST_DIR)/test_request_queue
	$(CC) $(CFLAGS) $(SRC) tests/test_worker_pool.c -o $(TEST_DIR)/test_worker_pool
	$(CC) $(CFLAGS) $(SRC) tests/test_http_server.c -o $(TEST_DIR)/test_http_server
	$(CC) $(CFLAGS) $(SRC) tests/test_http_connection.c -o $(TEST_DIR)/test_http_connection
	$(CC) $(CFLAGS) $(SRC) tests/test_http_concurrency.c -o $(TEST_DIR)/test_http_concurrency
	$(CC) $(CFLAGS) $(SRC) tests/test_http_request.c -o $(TEST_DIR)/test_http_request
	$(CC) $(CFLAGS) $(SRC) tests/test_http_dispatch.c -o $(TEST_DIR)/test_http_dispatch
	$(CC) $(CFLAGS) $(SRC) tests/test_http_response.c -o $(TEST_DIR)/test_http_response

	$(foreach test,$(TESTS),./$(TEST_DIR)/$(test);)

clean:
	rm -rf build
	rm -f *.db
