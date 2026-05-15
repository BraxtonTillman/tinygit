// main.c

#include "../include/add.h"
#include "../include/init.h"
#include <stdio.h>

int main(int argc, char *argv[1]) {
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