CC = clang

# Ask Homebrew where OpenSSL is — works on both arm64 (/opt/homebrew)
# and Intel (/usr/local) without hardcoding.
OPENSSL := $(shell brew --prefix openssl)

CFLAGS  = -Wall -Wextra -g -std=c11 -I include -I$(OPENSSL)/include
LDFLAGS = -L$(OPENSSL)/lib -lcrypto -lz

SRC = src/main.c src/init.c src/index.c src/add.c src/object.c
TARGET = tinygit

# Sources the test harness needs (no main.c/add.c — the harness has its own main)
TEST_SRC = src/index.c src/object.c src/init.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

test: $(TEST_SRC) tests/run_tests.c src/commit.c
	mkdir -p build
	$(CC) $(CFLAGS) tests/run_tests.c $(TEST_SRC) src/commit.c -o build/run_tests $(LDFLAGS)
	rm -rf build/testrepo && mkdir -p build/testrepo/.git/objects
	cd build/testrepo && ../run_tests

clean:
	rm -f $(TARGET)
	rm -rf build