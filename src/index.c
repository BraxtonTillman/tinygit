/* File: index.c
   Auth: Braxton Tillman
   Desc: This file will create an index
         with a read, modify, write,
         and free cycle. The goal is to
         index the objects being created
         or modified in add.c.
*/

#include "../include/index.h"

int read_index(const char *path, struct Index *out_index);
void add_entry(const struct Entry in_entry, struct Index *out_index);
int write_index(const char *path, const struct Index *index);
void free_index(struct Index *index);

// TODO: Start with free index because that is the easiest function.
//       Then work your way to write_index.