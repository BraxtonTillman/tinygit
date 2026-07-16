// main.c

#include "../include/add.h"
#include "../include/commit.h"
#include "../include/init.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Not enough commands.\n");
    return 1;
  }

  if (strcmp(argv[1], "init") == 0) {
    tinygitInit();
  }

  if (strcmp(argv[1], "add") == 0) {
    tinygitAdd(argv[2]);
  }

  if (strcmp(argv[1], "commit") == 0) {
    if (argc < 4 || strcmp(argv[2], "-m") != 0) {
      fprintf(stderr, "usage: tinygit commit -m <message>\n");
      return 1;
    }
    return tinygitCommit(argv[3]);
  }

  if (strcmp(argv[1], "log") == 0) {
    return tinygitLog();
  }

  fprintf(stderr, "unknown command: %s\n", argv[1]);
  return 1;
}