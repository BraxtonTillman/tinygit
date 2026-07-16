// object.c
#include "../include/object.h"
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

unsigned char *build_object(const char *type, const unsigned char *content,
                            size_t content_size, size_t *out_object_size) {
  char header[32];
  size_t header_len =
      snprintf(header, sizeof(header), "%s %zu", type, content_size);
  size_t header_size = header_len + 1;

  unsigned char *buffer = malloc(header_size + content_size);
  if (buffer == NULL)
    return NULL;

  memcpy(buffer, header, header_size);
  memcpy(buffer + header_size, content, content_size);

  *out_object_size = header_size + content_size;

  return buffer;
}

void hash_object(const unsigned char *blob_buffer, size_t blob_size,
                 unsigned char *out_hash) {
  SHA1(blob_buffer, blob_size, out_hash);
}

void build_hash(char *out_hex_hash, unsigned char *out_hash) {
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf(out_hex_hash + (i * 2), "%02x", out_hash[i]);
  }
}

unsigned char *compress_object(const unsigned char *blob_buffer,
                               size_t blob_size, size_t *out_compressed_size) {
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

int write_object(unsigned char *compressed_blob, size_t compressed_size,
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

int store_object(const char *type, const unsigned char *content,
                 size_t content_size, char out_hex[41],
                 unsigned char out_raw[20]) {
  size_t object_size = 0;
  size_t compressed_size = 0;
  unsigned char *object =
      build_object(type, content, content_size, &object_size);
  if (object == NULL) {
    fprintf(stderr, "When storing object, the object is NULL.");
    return -1;
  }

  hash_object(object, object_size, out_raw);
  build_hash(out_hex, out_raw);

  unsigned char *compressed_object =
      compress_object(object, object_size, &compressed_size);
  if (compressed_object == NULL) {
    fprintf(
        stderr,
        "When compressing the store object, the compressed object is NULL.");
    free(object);
    return -1;
  }

  int result = write_object(compressed_object, compressed_size, out_hex);
  free(object);
  free(compressed_object);

  return result;
}

int read_object(const char *hex, unsigned char *out_content, size_t bufsize,
                size_t *out_len) {
  char path[64];
  snprintf(path, sizeof(path), ".git/objects/%.2s/%s", hex, hex + 2);
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s.\n", path);
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  long size_fp = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size_fp < 0) {
    fprintf(stderr, "Size of file is NULL.\n");
    fclose(fp);
    return -1;
  }

  unsigned char *file_contents_buffer = malloc(size_fp + 1);

  if (file_contents_buffer == NULL) {
    fclose(fp);
    return -1;
  }

  size_t bytes_read = fread(file_contents_buffer, 1, size_fp, fp);

  if (bytes_read != (size_t)size_fp) {
    fprintf(stderr, "Object was partially read. Re-run command.\n");
    free(file_contents_buffer);
    fclose(fp);
    return -1;
  }

  uLongf dest_len = bufsize;

  int result = uncompress(out_content, &dest_len, file_contents_buffer,
                          (uLongf)bytes_read);

  free(file_contents_buffer);
  fclose(fp);

  if (result != Z_OK) {
    fprintf(stderr, "Decompression failed (zlib code %d)\n", result);
    return -1;
  }

  unsigned char *nul = memchr(out_content, '\0', dest_len);
  if (nul == NULL) {
    fprintf(stderr, "Malformed object: no header terminator\n");
    return -1;
  }

  size_t header_len = (nul - out_content) + 1;
  *out_len = dest_len - header_len;

  memmove(out_content, nul + 1, *out_len);

  if (*out_len < bufsize)
    out_content[*out_len] = '\0';

  return 0;
}

int tinygitLog() { return 0; }