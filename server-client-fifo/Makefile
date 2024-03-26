CC = gcc

CFLAGS = -Wall -Werror -Wextra

SERVER_EXEC = server/potentital-server
CLIENT_EXEC = client/potentital-client

SERVER_SRC = server/main.c
CLIENT_SRC = client/main.c

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Rule to build the server
$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the client
$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Clean the built executables
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)

