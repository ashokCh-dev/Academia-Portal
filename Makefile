CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

# Directory structure
SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
COMMON_DIR = $(SRC_DIR)/common

# Server source files
SERVER_SRC = $(SERVER_DIR)/server.c $(SERVER_DIR)/admin_handler.c \
             $(SERVER_DIR)/student_handler.c $(SERVER_DIR)/faculty_handler.c \
             $(SERVER_DIR)/auth.c $(SERVER_DIR)/file_ops.c $(COMMON_DIR)/utils.c

# Client source files
CLIENT_SRC = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/ui.c $(COMMON_DIR)/utils.c

# Output binaries
SERVER_BIN = server
CLIENT_BIN = client

# Default target
all: $(SERVER_BIN) $(CLIENT_BIN)

# Server compilation
$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client compilation
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -I$(COMMON_DIR)

# Clean build files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean