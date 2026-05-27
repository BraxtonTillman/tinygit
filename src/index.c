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
#include <openssl/sha.h>

int read_index(const char *path, struct Index *out_index);
void add_entry(const struct Entry in_entry, struct Index *out_index);
static void write_and_hash(const void *data, size_t len, FILE *fp, SHA_CTX *ctx) {
      fwrite(data, 1, len, fp);
      SHA1_Update(ctx, data, len);
}

int write_index(const char *path, const struct Index *index) {
      SHA_CTX ctx;
      unsigned char digest[20];
      uint32_t version = htonl(2);
      uint32_t count = htonl((uint32_t)(index->count));
      FILE *fp = fopen(path, "wb");
      if (fp == NULL) {
            fprintf(stderr,"Could not open %s\n", path);
            return -1;
      }
      
      // Initialize SHA state
      SHA1_Init(&ctx);

      // Write the 12 byte header
      write_and_hash("DIRC", 4, fp, &ctx);
      write_and_hash(&version, sizeof(version), fp, &ctx);
      write_and_hash(&count, sizeof(count), fp, &ctx);
      
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
            
            write_and_hash(&ctime_sec, sizeof(ctime_sec), fp, &ctx);
            write_and_hash(&ctime_nsec, sizeof(ctime_nsec), fp, &ctx);
            write_and_hash(&mtime_sec, sizeof(mtime_sec), fp, &ctx);
            write_and_hash(&mtime_nsec, sizeof(mtime_nsec), fp, &ctx);
            write_and_hash(&dev, sizeof(dev), fp, &ctx);
            write_and_hash(&ino, sizeof(ino), fp, &ctx);
            write_and_hash(&mode, sizeof(mode), fp, &ctx);
            write_and_hash(&uid, sizeof(uid), fp, &ctx);
            write_and_hash(&gid, sizeof(gid), fp, &ctx);
            write_and_hash(&file_size, sizeof(file_size), fp, &ctx);
            write_and_hash(e->sha1, 20, fp, &ctx);
            write_and_hash(&flags, sizeof(flags), fp, &ctx);
            write_and_hash(e->path, name_len +1, fp, &ctx);
            write_and_hash(zeros, pad_len, fp, &ctx);
            

      }

      // add trailing 20 byte hash checksum

      SHA1_Final(digest, &ctx);
      fwrite(digest, 1, 20, fp);

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