// index.h

#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <sys/stat.h>

struct Entry {
  struct stat st;
  unsigned char sha1[20];
  char path[256];
};

struct Index {
  size_t count;
  size_t capacity;
  struct Entry *entries;
};

// Four functions: Read, Modify, Write, Free

int read_index(const char *path, struct Index *out_index);
void add_entry(const struct Entry in_entry, struct Index *out_index);
int write_index(const char *path, const struct Index *index);
void free_index(struct Index *index);

#endif