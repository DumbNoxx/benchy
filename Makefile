CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -static
SRC = $(shell find src -name "*.c")
OBJ = $(SRC:.c=.o)
BIN = benchy
BUILD_DIR = build/

all: $(BIN)

$(BIN): $(OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $(BUILD_DIR)$@

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) src/*.o 

cas:
	echo "build"

.PHONY: all clean

