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
#include <errno.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
#define MAX_OBJECT_PATH_LEN_REST_HEX 64
#define MAX_OBJECT_PATH_LEN_FIRST_HEX 20
#define DIR_PERMS 0777

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

static unsigned char *build_blob(unsigned char *file_contents, size_t file_size,
                                 size_t *out_blob_size) {
  // Get header length and combine the header and content together
  char blob_header[32];
  size_t blob_header_len =
      snprintf(blob_header, sizeof(blob_header), "blob %zu", file_size);

  size_t header_size =
      blob_header_len + 1; // +1 is accounting for the null byte

  unsigned char *blob_buffer = malloc(header_size + file_size);
  if (blob_buffer == NULL) {
    return NULL;
  }

  memcpy(blob_buffer, blob_header, header_size);
  memcpy(blob_buffer + header_size, file_contents, file_size);

  *out_blob_size = header_size + file_size;

  return blob_buffer;
}

static void hash_blob(const unsigned char *blob_buffer, size_t blob_size,
                      unsigned char *out_hash) {
  SHA1(blob_buffer, blob_size, out_hash);
}

static void build_hash(char *out_hex_hash, unsigned char *out_hash) {
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf(out_hex_hash + (i * 2), "%02x", out_hash[i]);
  }
}

static unsigned char *compress_blob(const unsigned char *blob_buffer,
                                    size_t blob_size,
                                    size_t *out_compressed_size) {
  uLongf capacity = compressBound(blob_size);

  unsigned char *compressed_blob = malloc(capacity);

  if (compressed_blob == NULL) {
    return NULL;
  }

  int result = compress(compressed_blob, &capacity, blob_buffer, blob_size);

  if (result != Z_OK) {
    free(compressed_blob);
    return NULL;
  }

  *out_compressed_size = (size_t)capacity;

  return compressed_blob;
}

static int write_object(unsigned char *compressed_blob, size_t compressed_size,
                        const char *hex_hash) {

  // Variables
  char first_hex[MAX_OBJECT_PATH_LEN_FIRST_HEX];
  char rest_hex[MAX_OBJECT_PATH_LEN_REST_HEX];

  // Create directory using first 2 hex
  snprintf(first_hex, sizeof(first_hex), ".git/objects/%.2s", hex_hash);

  if (mkdir(first_hex, DIR_PERMS) == -1) {
    if (errno != EEXIST) {
      perror("Failed to create folder for a different reason.");
      return -1;
    }
  }

  // Write file of compressed bytes to new file (named the rest of the hex)
  snprintf(rest_hex, sizeof(rest_hex), ".git/objects/%.2s/%s", hex_hash,
           hex_hash + 2);

  FILE *fp = fopen(rest_hex, "wb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", hex_hash);
    return -1;
  }

  size_t written = fwrite(compressed_blob, 1, compressed_size, fp);
  if (written != compressed_size) {
    fprintf(stderr, "Short write to object file\n");
    fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    fprintf(stderr, "Failed to flush object file\n");
    return -1;
  }

  return 0;
}

int tinygitAdd(const char *path) {
  // Variables
  size_t file_size = 0;
  size_t blob_size = 0;
  size_t compressed_size = 0;
  size_t write_result = 0;
  char hex_hash[41];
  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char *file = NULL;
  unsigned char *blob = NULL;
  unsigned char *compressed_blob = NULL;

  file = read_file(path, &file_size);
  if (file == NULL)
    return -1;

  blob = build_blob(file, file_size, &blob_size);
  if (blob == NULL) {
    free(file);
    return -1;
  }

  // Function Calls
  hash_blob(blob, blob_size, hash);

  build_hash(hex_hash, hash);

  compressed_blob = compress_blob(blob, blob_size, &compressed_size);
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