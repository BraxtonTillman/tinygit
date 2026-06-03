// main.c

#include "../include/add.h"
#include "../include/index.h"
#include "../include/init.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Not enough commands.\n");
    return -1;
  }

  if (strcmp(argv[1], "init") == 0) {
    tinygitInit();
  }

  if (strcmp(argv[1], "add") == 0) {
    tinygitAdd(argv[2]);
  }

  return 0;
}

/* int main(void) {
 struct Index idx = {0};
 struct Entry e = {0};
 stat("foo.txt", &e.st);
 strcpy(e.path, "foo.txt");
 idx.entries = &e;
 idx.count = 1;
 write_index(".git/index", &idx);
 return 0;
} */

/* int main(void) {
  struct Index idx = {0};
  read_index(".git/index", &idx);
  write_index("roundtrip.index", &idx);
  free_index(&idx);
} */

/* int main(void) {
  struct Index idx = {.entries = NULL, .count = 0, .capacity = 0};

  struct Entry a = {0}; // {0} zeroes the whole struct — stat, sha1, all of it
  struct Entry b = {0};
  struct Entry c = {0};
  struct Entry d = {0};
  struct Entry e = {0};

  strcpy(a.path, "src/main.c");
  strcpy(b.path, "src/index.c");
  strcpy(c.path, "src/add.c");
  strcpy(d.path, "src/init.c");
  strcpy(e.path, "include/index.h");

  // --- Test 1: three distinct adds → count should be 3 ---
  add_entry(&a, &idx);
  add_entry(&b, &idx);
  add_entry(&c, &idx);

  printf("after 3 distinct: count=%zu (expect 3)\n", idx.count);

  // --- Test 2: re-add an existing path → count should NOT grow ---

  b.sha1[0] = 0xAB;
  add_entry(&b, &idx);
  for (size_t i = 0; i < idx.count; i++) {
    if (strcmp(idx.entries[i].path, "src/index.c") == 0) {
      printf("replaced b sha1[0]=0x%02X (expect 0xAB)\n",
             idx.entries[i].sha1[0]);
    }
  }

  // --- Test 3: cross the capacity-4 boundary ---
  add_entry(&d, &idx);
  printf("after 4: count=%zu capacity=%zu (expect cap 4)\n", idx.count,
         idx.capacity);
  add_entry(&e, &idx);
  printf("after 5: count=%zu capacity=%zu (expect cap 8)\n", idx.count,
         idx.capacity);

  free_index(&idx); // and run the whole thing under valgrind
  return 0;
} */