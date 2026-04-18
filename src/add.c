/* File: add.c
   Auth: Braxton Tillman
   Desc: This file will take the contents of
         a file, compute a hash of it, and
         store the content inside .git/objects/
         as a blob object. It will then update
         the staging area as a record.
*/

// PREPROCESSOR DIRECTIVES
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
// FORWARD DELCARATIONS

// PRACTICE FUNCTION
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Not enough commands.");
    return 1;
  }

  // Open a file
  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", argv[1]);
    return 1;
  }

  // Read its bytes into a buffer
  fseek(fp, 0, SEEK_END); // jump to end
  long size = ftell(fp);  // where im at == size
  fseek(fp, 0, SEEK_SET); // go back to start

  unsigned char *file_contents = malloc(size); // input buffer for SHA-1
  unsigned char hash[SHA224_DIGEST_LENGTH];    // output buffer

  size_t bytes_read = fread(file_contents, 1, size, fp);

  SHA1((const unsigned char *)file_contents, bytes_read, hash);

  // Print hex string
  for (size_t i = 0; i < SHA224_DIGEST_LENGTH; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");

  // Cleanup
  fclose(fp);
  free(file_contents);

  return 0;
}

// WORKFLOW:
/*  PART 1
    1. Read file content into a buffer
    2. Build the blob string: blob <size>\0<content>
    3. Hash that whole thing with SHA-1 which gives object ID.
    4. Zlib-compress that same blob string.
    5. Write the compressed bytes to .git/objects/<first-2>/<last 38>
*/
/*  PART 2
    1. Create the blob object (this is from part 1)
    2. Read the existing index.
    3. Add or replace the entry for that path with the new hash
    4. Write the index back.
*/