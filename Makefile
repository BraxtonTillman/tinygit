CC = clang

OPENSSL := $(shell brew --prefix openssl)

CFLAGS  = -Wall -Wextra -g -std=c11 -I include -I$(OPENSSL)/include
LDFLAGS = -L$(OPENSSL)/lib -lcrypto -lz

SRC  = src/main.c src/init.c src/index.c src/add.c src/object.c src/commit.c
HDRS = include/object.h include/add.h include/index.h include/commit.h include/init.h
TARGET = tinygit

TEST_SRC = src/index.c src/object.c src/init.c

.PHONY: test clean install

$(TARGET): $(SRC) $(HDRS)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

test: $(TEST_SRC) $(HDRS) tests/run_tests.c src/commit.c
	mkdir -p build
	$(CC) $(CFLAGS) tests/run_tests.c $(TEST_SRC) src/commit.c -o build/run_tests $(LDFLAGS)
	rm -rf build/testrepo && mkdir -p build/testrepo/.git/objects
	cd build/testrepo && ../run_tests

clean:
	rm -f $(TARGET)
	rm -rf build

PREFIX ?= /usr/local
install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/tinygit