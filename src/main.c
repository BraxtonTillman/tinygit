// main.c

#include "../include/add.h"
#include "../include/init.h"
#include "../include/index.h"
#include <stdio.h>
#include <string.h>

/* int main(int argc, char *argv[]) {
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
} */

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

int main(void) {
  struct Index idx = {0};
  read_index(".git/index", &idx);
  write_index("roundtrip.index",&idx);
  free_index(&idx);
}