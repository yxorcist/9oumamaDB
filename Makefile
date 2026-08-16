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

clean:
	rm -rf $(OUT)
