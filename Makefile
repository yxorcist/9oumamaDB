CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -g

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

	./test_page_manager
	./test_buffer_pool
	./test_storage
	./test_db
	./test_heap
	./test_parser

clean:
	rm -rf build
	rm -f test_page_manager test_buffer_pool test_storage test_db test_heap test_parser *.db
