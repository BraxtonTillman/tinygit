// commit.c

#include "../include/index.h"
#include <complex.h>
#include <stdio.h>
#include <string.h>

#define NULL_BYTE = 1
#define MODE_BYTE = 6
#define SHA_HASH = 20

int write_tree(struct Entry *e) {
  // Create buffer for rows
  char row[300];

  // n is "100644 x" which equals 8
  int n = sprintf(row, "100644 %s", e->path);

  // We then append the raw 20 sha bytes at offset (it's 9 because n is 8 not
  // including the NUL term)
  memcpy(row + n + 1, e->sha1, 20);

  size_t row_len = n + 1 + 20;
  return 0;
}