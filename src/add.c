/* File: add.c
   Auth: Braxton Tillman
   Desc: This file will take the contents of
         a file, compute a hash of it, and
         store the content inside .git/objects/
         as a blob object. It will then update
         the staging area as a record.
*/

// PREPROCESSOR DIRECTIVES
#include <_string.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

unsigned char *read_file(const char *path, size_t *out_size);
unsigned char *build_blob(unsigned char *file_contents, size_t file_size,
                          size_t *out_blob_size);
void hash_blob(const unsigned char *blob_buffer, size_t blob_size,
               unsigned char *out_hash);
void print_hash(unsigned char *out_hash);

int main(int argc, char *argv[1]) {

  if (argc < 2) {
    printf("Not enough commands.");
    return 1;
  }

  size_t file_size;
  size_t blob_size;
  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char *file = read_file(argv[1], &file_size);

  // TODO: call other functions, clean up mallocs, and test
  unsigned char *blob = build_blob(file, file_size, &blob_size);

  hash_blob(blob, blob_size, hash);
  print_hash(hash);

  // Cleanup
  free(file);
  free(blob);

  return 0;
}

unsigned char *read_file(const char *path, size_t *out_size) {
  // Open file
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", path);
    return NULL;
  }

  // Get size of file
  fseek(fp, 0, SEEK_END);   // jump to end
  long size_fp = ftell(fp); // where im at == size
  fseek(fp, 0, SEEK_SET);   // go back to start

  // Allocate buffer & read file bytes
  unsigned char *file_contents_buffer = malloc(size_fp);
  size_t bytes_read = fread(file_contents_buffer, 1, size_fp, fp);
  *out_size = bytes_read;

  fclose(fp);
  return file_contents_buffer;
}

unsigned char *build_blob(unsigned char *file_contents, size_t file_size,
                          size_t *out_blob_size) {
  // Get header length and combine the header and content together
  char blob_header[32];
  size_t blob_header_len =
      snprintf(blob_header, sizeof(blob_header), "blob %zu", file_size);

  size_t header_size =
      blob_header_len + 1; // +1 is accounting for the null byte

  unsigned char *blob_buffer = malloc(header_size + file_size);

  memcpy(blob_buffer, blob_header, header_size);
  memcpy(blob_buffer + header_size, file_contents, file_size);

  *out_blob_size = header_size + file_size;

  return blob_buffer;
}

void hash_blob(const unsigned char *blob_buffer, size_t blob_size,
               unsigned char *out_hash) {
  SHA1(blob_buffer, blob_size, out_hash);
}

void print_hash(unsigned char *out_hash) {
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    printf("%02x", out_hash[i]);
  }
  printf("\n");
}

// WORKFLOW:
/*  PART 1
    1. Read file content into a buffer DONE
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