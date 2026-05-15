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
#define OBJECT_PATH_LEN 64

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

void build_hash(char *out_hex_hash, unsigned char *out_hash) {
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf(out_hex_hash + (i * 2), "%02x", out_hash[i]);
  }
}

void print_hash(unsigned char *out_hash) {
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    printf("%02x", out_hash[i]);
  }
  printf("\n");
}

unsigned char *compress_blob(const unsigned char *blob_buffer, size_t blob_size,
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

void write_object(unsigned char *compressed_blob, size_t compressed_size,
                  const char *hex_hash) {

  // Variables
  char first_hex[OBJECT_PATH_LEN];
  char rest_hex[OBJECT_PATH_LEN];
  const int DIR_PERMS = 0777;

  // Create directory using first 2 hex
  snprintf(first_hex, sizeof(first_hex), ".git/objects/%.2s", hex_hash);

  if (mkdir(first_hex, DIR_PERMS) == -1) {
    if (errno != EEXIST) {
      perror("Failed to create folder for a different reason.");
    }
  }

  // Write file of compressed bytes to new file (named the rest of the hex)
  snprintf(rest_hex, sizeof(rest_hex), ".git/objects/%.2s/%s", hex_hash,
           hex_hash + 2);

  FILE *fp = fopen(rest_hex, "wb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", hex_hash);
  }

  fwrite(compressed_blob, 1, compressed_size, fp);

  fclose(fp);
}

int tinygitAdd(const char *path) {
  // Variables
  size_t file_size;
  size_t blob_size;
  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char *file = read_file(path, &file_size);
  char hex_hash[41];
  size_t compressed_size;

  unsigned char *blob = build_blob(file, file_size, &blob_size);

  // Function Calls
  hash_blob(blob, blob_size, hash);
  print_hash(hash);
  build_hash(hex_hash, hash);

  unsigned char *compressed_blob =
      compress_blob(blob, blob_size, &compressed_size);

  write_object(compressed_blob, compressed_size, hex_hash);

  // Cleanup
  free(file);
  free(blob);
  free(compressed_blob);

  return 0;
}