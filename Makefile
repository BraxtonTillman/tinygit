CC = clang
CFLAGS = -Wall -Wextra -g -std=c11 -I include

SRC = src/main.c src/init.c
TARGET = tinygit

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
