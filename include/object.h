#ifndef OBJECT_H
#define OBJECT_H
#include <stddef.h>

unsigned char *build_object(const char *type, const unsigned char *content,
                            size_t content_size, size_t *out_object_size);
void hash_object(const unsigned char *blob_buffer, size_t blob_size,
                 unsigned char *out_hash);
void build_hash(char *out_hex_hash, unsigned char *out_hash);
unsigned char *compress_object(const unsigned char *blob_buffer,
                               size_t blob_size, size_t *out_compressed_size);
int write_object(unsigned char *compressed_blob, size_t compressed_size,
                 const char *hex_hash);
int store_object(const char *type, const unsigned char *content,
                 size_t content_size, char out_hex[41],
                 unsigned char out_raw[20]);
int read_object(const char *hex, unsigned char *out_content, size_t bufsize,
                size_t *out_len);

#endif