#ifndef COMMIT_H
#define COMMIT_H
#include <time.h>

int build_commit_buffer(const char *tree_hex, const char *parent_hex,
                        const char *message, time_t ts, char *buf,
                        size_t bufsize);

int write_commit(const char *tree_hex, const char *parent_hex,
                 const char *message, char out_hex[41],
                 unsigned char out_raw[20]);

int write_tree(struct Index *index, char out_hex[41],
               unsigned char out_raw[20]);

#endif