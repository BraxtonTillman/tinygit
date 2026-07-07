#include "../include/index.h"
#include "../include/object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int write_tree(struct Index *index, char out_hex[41],
               unsigned char out_raw[20]) {
  // PASS 1: compute exact total size
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

  // PASS 2: build each row, append into tree_buf
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

  // store: pass the CALLER's out_hex/out_raw straight through
  int rc = store_object("tree", tree_buf, offset, out_hex, out_raw);

  free(tree_buf);
  return rc;
}