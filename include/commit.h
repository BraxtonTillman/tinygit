#ifndef COMMIT_H
#define COMMIT_H
#include "index.h"
#include <stddef.h>
#include <time.h>

int build_commit_buffer(const char *tree_hex, const char *parent_hex,
                        const char *message, time_t ts, char *buf,
                        size_t bufsize);

int write_commit(const char *tree_hex, const char *parent_hex,
                 const char *message, char out_hex[41],
                 unsigned char out_raw[20]);

int write_tree(struct Index *index, char out_hex[41],
               unsigned char out_raw[20]);
int read_head_ref(const char *head_path, char *out_ref, size_t size);
int read_ref(const char *ref_path, char out_hex[41]);
int update_ref(const char *ref_path, const char *hex);
int parse_commit(const unsigned char *buf, const size_t len, char out_tree[41],
                 char out_parent[41], char *out_msg, size_t msg_size);
int tinygitCommit(const char *message);
int tinygitLog(void);
#endif