// init.c

// PREPROCESSOR DIRECTIVES 
#include "../include/init.h"
#include <stdio.h>
#include <sys/stat.h>
#define MAX_PATH_LEN 256

// FORWARD DECLARATIONS 
static int createInitFile(const char *fileName, const char *fileContent);


// This function creates the .git/ directory and all files and subdirectories inside
int tinygitInit(void) {

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

  // Directory Creation
  for (size_t i = 0; i < num_directories; i++) {

    if (mkdir(directories[i], DIR_PERMS) == -1) {
      perror("Error creating directory.");
      return -1;
    }
  }

  printf("Directories created successfully.\n");

  // File Creation
  if (createInitFile("HEAD","ref: refs/heads/master") == -1) {
    return -1;
  }

  if (createInitFile("description","Unnamed repository; edit this file 'description' to name the repository.") == -1) {
    return -1;
  }

  if (createInitFile("config","[core]\n"
	"\trepositoryformatversion = 0\n"
	"\tfilemode = true\n"
	"\tbare = false\n"
	"\tlogallrefupdates = true\n"
	"\tignorecase = true\n"
	"\tprecomposeunicode = true\n") == -1) {
    return -1;
  }  
  
  return 0;
}

// This function creates the files and the content inside each file for the tinygitInit function
static int createInitFile(const char *fileName, const char *fileContent) {
  char path[MAX_PATH_LEN]; 
  FILE *fptr;
  
  snprintf(path, sizeof(path), ".git/%s", fileName);

  fptr = fopen(path, "w");

  if (fptr != NULL) {
    fprintf(fptr, "%s", fileContent);
  
    fclose(fptr);

  }
  else {
    perror("Error creating file.");
    return -1;
  }
  return 0;

}
