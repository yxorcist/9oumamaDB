CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -pthread -g

SANITIZER_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
TEST_DIR = $(BUILD_DIR)/tests

SRC = $(filter-out src/main.c,$(wildcard src/*.c))
OBJ = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC))

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
	test_http_response \
	test_persistence \
	test_http_fragmentation

.PHONY: all run test clean

all: $(BUILD_DIR)/9oumamaDB

$(BUILD_DIR)/9oumamaDB: $(OBJ) src/main.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) src/main.c -o $@

$(OBJ_DIR)/%.o: src/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DIR)/%: tests/%.c $(OBJ)
	mkdir -p $(TEST_DIR)
	$(CC) $(CFLAGS) $(OBJ) $< -o $@

test: $(addprefix $(TEST_DIR)/,$(TESTS))
	@for test in $^; do \
		echo "=== $$test ==="; \
		./$$test || exit 1; \
	done

sanitize: CFLAGS += $(SANITIZER_FLAGS)
sanitize: clean test

run: all
	./$(BUILD_DIR)/9oumamaDB

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.db
