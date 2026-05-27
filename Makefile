CC = clang
CFLAGS = -Wall -Wextra -g -std=c11 -I include -I/opt/homebrew/opt/openssl/include
LDFLAGS = -L/opt/homebrew/opt/openssl/lib -lcrypto -lz

SRC = src/main.c src/init.c src/index.c src/add.c
TARGET = tinygit

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)