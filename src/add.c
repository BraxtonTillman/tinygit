/* File: add.c
   Auth: Braxton Tillman
   Desc: This file will take the contents of
         a file, compute a hash of it, and
         store the content inside .git/objects/
         as a blob object. It will then update
         the staging area as a record.
*/

// PREPROCESSOR DIRECTIVES
#include "../include/add.h"
#include "../include/index.h"
#include "../include/object.h"
#include <errno.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

static unsigned char *read_file(const char *path, size_t *out_size) {
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

  if (size_fp < 0) {
    fprintf(stderr, "Size of file is NULL.\n");
    fclose(fp);
    return NULL;
  }

  // Allocate buffer & read file bytes
  unsigned char *file_contents_buffer = malloc(size_fp + 1);
  if (file_contents_buffer == NULL) {
    fclose(fp);
    return NULL;
  }

  size_t bytes_read = fread(file_contents_buffer, 1, size_fp, fp);
  *out_size = bytes_read;

  if (bytes_read != (size_t)size_fp) {
    fprintf(stderr, "Blob object was partially hashed. Re-run command.\n");
    free(file_contents_buffer);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  return file_contents_buffer;
}

int tinygitAdd(const char *path) {
  // Variables
  size_t content_size = 0;
  size_t blob_size = 0;
  size_t compressed_size = 0;
  size_t write_result = 0;
  char hex_hash[41];
  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char *file = NULL;
  unsigned char *blob = NULL;
  unsigned char *compressed_blob = NULL;

  file = read_file(path, &content_size);
  if (file == NULL)
    return -1;

  blob = build_object("blob",file, content_size, &blob_size);
  if (blob == NULL) {
    free(file);
    return -1;
  }

  // Function Calls
  hash_object(blob, blob_size, hash);

  build_hash(hex_hash, hash);

  compressed_blob = compress_object(blob, blob_size, &compressed_size);
  if (compressed_blob == NULL) {
    fprintf(stderr, "Compressed Blob is NULL.\n");
    free(file);
    free(blob);
    return -1;
  }

  write_result = write_object(compressed_blob, compressed_size, hex_hash);
  if (write_result != 0) {
    fprintf(stderr, "File was not written.\n");
    free(file);
    free(blob);
    free(compressed_blob);
    return -1;
  }

  struct stat st;

  if (stat(path, &st) != 0) {
    perror("stat failed");
    free(file);
    free(blob);
    free(compressed_blob);
    return -1;
  }

  struct Entry e;

  e.st = st;
  memcpy(e.sha1, hash, SHA_DIGEST_LENGTH);
  strncpy(e.path, path, sizeof(e.path) - 1);
  e.path[sizeof(e.path) - 1] = '\0';

  struct Index idx = {0};

  // read
  if (read_index(".git/index", &idx) != 0) {
    fprintf(stderr, "Index is not reading...\n");
    free(file);
    free(blob);
    free(compressed_blob);
    return -1;
  }
  // add
  if (add_entry(&e, &idx) != 0) {
    fprintf(stderr, "Entry is not added to index...\n");
    free_index(&idx);
    free(file);
    free(blob);
    free(compressed_blob);
    return -1;
  }

  // write
  if (write_index(".git/index", &idx) != 0) {
    fprintf(stderr, "Can not write index to %s\n", path);
    free_index(&idx);
    free(file);
    free(blob);
    free(compressed_blob);
    return -1;
  }

  // free
  free_index(&idx);

  // Cleanup
  free(file);
  free(blob);
  free(compressed_blob);

  return 0;
}

// TODO: collapse error cleanup to a single goto cleanup label