CC = gcc
CFLAGS = -Wall -Werror -Wextra -ansi -pedantic
TARGET = main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

# include --leak-check=full flag for detailed memory check
valgrind: $(TARGET)
	valgrind ./$(TARGET)

clean:
	rm -f $(TARGET)