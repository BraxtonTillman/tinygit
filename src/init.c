// init.c

#include "../include/init.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

int tinygit_init(void) {

  // Variables
  const int DIR_PERMS = 0777;
  char *directories[] = {".git/",
                         ".git/objects/",
                         ".git/refs/",
                         ".git/objects/info/",
                         ".git/objects/pack/",
                         ".git/refs/heads/",
                         ".git/refs/tags/"};
  size_t num_directories = sizeof(directories) / sizeof(directories[0]);
  FILE *HEAD;
  FILE *config;
  FILE *description;

  // DIRECTORIES
  for (size_t i = 0; i < num_directories; i++) {

    if (mkdir(directories[i], DIR_PERMS) == -1) {
      printf("Error creating directory\n");
      return -1;
    }
  }

  printf("Directories created successfully.\n");

  // FILES

  HEAD = fopen("HEAD", "w");

  fprintf(HEAD, "ref: refs/heads/master\n");

  fclose(HEAD);

  return 0;
}

// DIRECTORIES
// .git
// .git/objects
// .git/objects/info
// .git/objects/pack
// .git/refs
// .git/refs/heads
// .git/refs/tags

// FILES
// config
// HEAD
// description
