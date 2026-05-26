/* File: index.c
   Auth: Braxton Tillman
   Desc: This file will create an index
         with a read, modify, write,
         and free cycle. The goal is to
         index the objects being created
         or modified in add.c.
*/

#include "../include/index.h"
#include <stdio.h>
#include <arpa/inet.h> // for htonl
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int read_index(const char *path, struct Index *out_index);
void add_entry(const struct Entry in_entry, struct Index *out_index);
int write_index(const char *path, const struct Index *index) {
      uint32_t version = htonl(2);
      uint32_t count = htonl((uint32_t)(index->count));
      FILE *fp = fopen(path, "wb");
      if (fp == NULL) {
            fprintf(stderr,"Could not open %s\n", path);
            return -1;
      }

      // Write the 12 byte header
      fwrite("DIRC", 1, 4, fp);
      fwrite(&version, sizeof(version), 1, fp);
      fwrite(&count, sizeof(count), 1, fp);
      
      // loop entries
      for (size_t i = 0; i < index->count; i++) {
            const struct Entry *e = &index->entries[i];

            size_t name_len = strlen(e->path);
            size_t entry_len = 62 + name_len + 1; // 62 bytes + path + NUL term
            size_t pad_len = 8 - (entry_len % 8);

            static const char zeros[8] = {0};

            uint32_t ctime_sec = htonl((uint32_t)e->st.st_ctimespec.tv_sec);
            uint32_t ctime_nsec = htonl((uint32_t)e->st.st_ctimespec.tv_nsec);
            uint32_t mtime_sec = htonl((uint32_t)e->st.st_mtimespec.tv_sec);
            uint32_t mtime_nsec = htonl((uint32_t)e->st.st_mtimespec.tv_nsec);
            uint32_t dev = htonl((uint32_t)e->st.st_dev);
            uint32_t ino = htonl((uint32_t)e->st.st_ino);
            uint32_t mode = htonl((uint32_t)e->st.st_mode);
            uint32_t uid = htonl((uint32_t)e->st.st_uid);
            uint32_t gid = htonl((uint32_t)e->st.st_gid);
            uint32_t file_size = htonl((uint32_t)e->st.st_size);

            uint16_t flags = htons(strlen(e->path) & 0xFFF);
            
            fwrite(&ctime_sec, sizeof(ctime_sec), 1, fp);
            fwrite(&ctime_nsec, sizeof(ctime_nsec), 1, fp);
            fwrite(&mtime_sec, sizeof(mtime_sec), 1, fp);
            fwrite(&mtime_nsec, sizeof(mtime_nsec), 1, fp);
            fwrite(&dev, sizeof(dev), 1, fp);
            fwrite(&ino, sizeof(ino), 1, fp);
            fwrite(&mode, sizeof(mode), 1, fp);
            fwrite(&uid, sizeof(uid), 1, fp);
            fwrite(&gid, sizeof(gid), 1, fp);
            fwrite(&file_size, sizeof(file_size), 1, fp); // 4 x 10
            fwrite(e->sha1, 1, 20, fp); // 20
            fwrite(&flags, sizeof(flags), 1, fp); // 2
            fwrite(e->path, 1, name_len + 1, fp);
            fwrite(zeros, 1, pad_len, fp);

      }

      /* TODO: add trailing 20 byte hash checksum
               Create helper function to write and hash per fwrite
               so like:
               fwrite();
               SHA1_Update() into one function for the for loop */


      fclose(fp);
      return 0;
}


void free_index(struct Index *index){
      if (index == NULL) return;
      free(index->entries);
      index->entries = NULL;
      index->count = 0;
      index->capacity = 0;
}

