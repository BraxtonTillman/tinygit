// add.h

#ifndef ADD_H
#define ADD_H

#include <errno.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

int tinygitAdd(const char *path);

unsigned char *read_file(const char *path, size_t *out_size);
unsigned char *build_blob(unsigned char *file_contents, size_t file_size,
                          size_t *out_blob_size);
void hash_blob(const unsigned char *blob_buffer, size_t blob_size,
               unsigned char *out_hash);
void print_hash(unsigned char *out_hash);

void build_hash(char *out_hex_hash, unsigned char *out_hash);

unsigned char *compress_blob(const unsigned char *blob_buffer, size_t blob_size,
                             size_t *out_compressed_size);

void write_object(unsigned char *compressed_blob, size_t compressed_size,
                  const char *hex_hash);

#endif