CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
SRC_DIR = .
OBJ_DIR = obj
INCLUDE_DIR = .

SRC_FILES = $(SRC_DIR)/echo_server.c
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

TARGET = echo_server

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
