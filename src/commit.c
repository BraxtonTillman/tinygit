#include "../include/index.h"
#include "../include/object.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int write_tree(struct Index *index, char out_hex[41],
               unsigned char out_raw[20]) {

  size_t total = 0;
  for (size_t i = 0; i < index->count; i++) {
    size_t name_len = strlen(index->entries[i].path);
    total += 6 + 1 + name_len + 1 + 20; // "100644" + ' ' + name + '\0' + sha
  }

  unsigned char *tree_buf = malloc(total);
  if (tree_buf == NULL) {
    fprintf(stderr, "write_tree: out of memory\n");
    return -1;
  }

  char scratch[300];
  size_t offset = 0;
  for (size_t i = 0; i < index->count; i++) {
    struct Entry *e = &index->entries[i];
    int n = sprintf(scratch, "100644 %s", e->path);
    memcpy(scratch + n + 1, e->sha1, 20);
    size_t row_len = (size_t)n + 1 + 20;

    memcpy(tree_buf + offset, scratch, row_len);
    offset += row_len;
  }

  int rc = store_object("tree", tree_buf, offset, out_hex, out_raw);

  free(tree_buf);
  return rc;
}

int build_commit_buffer(const char *tree_hex, const char *parent_hex,
                        const char *message, time_t ts, char *buf,
                        size_t bufsize) {
  int len = 0;
  const char *name = "Braxton Tillman";
  const char *email = "braxtontillman@gmail.com";

  len += snprintf(buf + len, bufsize - len, "tree %s\n", tree_hex);
  if (parent_hex)
    len += snprintf(buf + len, bufsize - len, "parent %s\n", parent_hex);
  len += snprintf(buf + len, bufsize - len, "author %s <%s> %ld +0000\n", name,
                  email, (long)ts);
  len += snprintf(buf + len, bufsize - len, "committer %s <%s> %ld +0000\n",
                  name, email, (long)ts);
  len += snprintf(buf + len, bufsize - len, "\n%s\n", message);
  return len;
}

int write_commit(const char *tree_hex, const char *parent_hex,
                 const char *message, char out_hex[41],
                 unsigned char out_raw[20]) {
  char buf[4096];
  int len = build_commit_buffer(tree_hex, parent_hex, message, time(NULL), buf,
                                sizeof(buf));
  return store_object("commit", (unsigned char *)buf, (size_t)len, out_hex,
                      out_raw);
}

int read_head_ref(const char *head_path, char *out_ref, size_t size) {
  FILE *fp = fopen(head_path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", head_path);
    return -1;
  }

  char buf[256];

  if (fgets(buf, sizeof(buf), fp) == NULL) {
    fprintf(stderr, "HEAD is empty\n");
    fclose(fp);
    return -1;
  }
  fclose(fp);

  buf[strcspn(buf, "\n")] = '\0';

  if (strncmp(buf, "ref: ", 5) != 0) {
    fprintf(stderr, "HEAD is not a symbolic ref (detached?)\n");
    return -1;
  }

  snprintf(out_ref, size, "%s", buf + 5);
  return 0;
}

int read_ref(const char *ref_path, char out_hex[41]) {
  FILE *fp = fopen(ref_path, "rb");
  if (fp == NULL) {
    if (errno == ENOENT)
      return 1;
    return -1;
  }
  char buf[50];

  if (fgets(buf, sizeof(buf), fp) == NULL) {
    fprintf(stderr, "Empty or unreadable ref: %s\n", ref_path);
    fclose(fp);
    return -1;
  }

  buf[strcspn(buf, "\n")] = '\0';

  if (strlen(buf) != 40) {
    fprintf(stderr, "Corrupt ref (expected 40 hex chars): %s\n", ref_path);
    fclose(fp);
    return -1;
  }

  snprintf(out_hex, 41, "%s", buf);

  fclose(fp);
  return 0;
}

int update_ref(const char *ref_path, const char *hex) {
  FILE *fp = fopen(ref_path, "w");
  if (fp == NULL) {
    return -1;
  }
  fprintf(fp, "%s\n", hex);
  if (fclose(fp) != 0) {
    return -1;
  }
  return 0;
}

int tinygitCommit(const char *message) {
  struct Index idx = {0};
  char tree_hex[41], commit_hex[41], parent_hex[41];
  unsigned char tree_raw[20], commit_raw[20];
  char ref_rel[256], ref_path[300];

  if (read_index(".git/index", &idx) != 0)
    return -1;
  if (idx.count == 0) {
    fprintf(stderr, "nothing to commit\n");
    free_index(&idx);
    return -1;
  }

  if (write_tree(&idx, tree_hex, tree_raw) != 0) {
    free_index(&idx);
    return -1;
  }
  free_index(&idx); // done with the index

  if (read_head_ref(".git/HEAD", ref_rel, sizeof(ref_rel)) != 0)
    return -1;
  snprintf(ref_path, sizeof(ref_path), ".git/%s", ref_rel);

  int rc = read_ref(ref_path, parent_hex);
  if (rc == -1)
    return -1;
  const char *parent = (rc == 0) ? parent_hex : NULL;

  if (write_commit(tree_hex, parent, message, commit_hex, commit_raw) != 0)
    return -1;
  if (update_ref(ref_path, commit_hex) != 0)
    return -1;

  printf("%s %s\n", (parent ? "commit" : "root-commit"), commit_hex);
  return 0;
}

int parse_commit(const unsigned char *buf, const size_t len, char out_tree[41],
                 char out_parent[41], char *out_msg, size_t msg_size) {

  size_t pos = 0;

  // tree line
  if (len < pos + 46) { /* too short */
    return -1;
  }
  if (strncmp((const char *)buf + pos, "tree ", 5) != 0) { /* error */
    return -1;
  }
  snprintf(out_tree, 41, "%.40s", buf + pos + 5);
  pos += 46;

  // parent line
  if (len >= pos + 7 && strncmp((const char *)buf + pos, "parent ", 7) == 0) {
    if (len < pos + 48) { /* too short */
      return -1;
    }
    snprintf(out_parent, 41, "%.40s", buf + pos + 7);
    pos += 48;
  } else {
    out_parent[0] = '\0';
  }

  // commit line
  const char *sep = strstr((const char *)buf + pos, "\n\n");
  if (sep == NULL) {
    fprintf(stderr, "Malformed commit: no header/message separator\n");
    return -1;
  }

  size_t msg_start = (size_t)(sep - (const char *)buf) + 2;
  size_t msg_len = len - msg_start;

  if (msg_len > 0 && buf[len - 1] == '\n')
    msg_len--;

  snprintf(out_msg, msg_size, "%.*s", (int)msg_len, buf + msg_start);

  return 0;
}
