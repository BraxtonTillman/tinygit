/* File: index.c
   Auth: Braxton Tillman
   Desc: This file will create an index
         with a read, modify, write,
         and free cycle. The goal is to
         index the objects being created
         or modified in add.c.
*/

#include "../include/index.h"
#include <arpa/inet.h>
#include <errno.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read_be16(FILE *fp) {
  uint16_t raw;
  fread(&raw, 1, 2, fp);
  return ntohs(raw);
}
static uint32_t read_be32(FILE *fp) {
  uint32_t raw;
  fread(&raw, 1, 4, fp);
  return ntohl(raw);
}

int read_index(const char *path, struct Index *out_index) {
  FILE *fp = fopen(path, "rb");
  unsigned char sig_buff[4];
  struct Entry *entries_buffer = NULL;
  uint32_t version = 0;
  uint32_t count = 0;
  unsigned char DIRC[4] = {'D', 'I', 'R', 'C'};
  size_t bytes_read = 0;

  if (fp == NULL) {
    if (errno == ENOENT) {
      out_index->count = 0;
      out_index->capacity = 0;
      out_index->entries = NULL;
      return 0;
    }

    fprintf(stderr, "No such file: %s\n", path);
    return -1;
  }

  bytes_read = fread(sig_buff, 1, 4, fp);
  if (bytes_read != 4) {
    fprintf(stderr, "File being read is too short: %s\n", path);
    fclose(fp);
    return -1;
  }

  if (memcmp(sig_buff, DIRC, 4) != 0) {
    fprintf(stderr, "FIle is corrupt: %s\n", path);
    fclose(fp);
    return -1;
  }

  bytes_read = fread(&version, 1, 4, fp);
  if (bytes_read != 4) {
    fprintf(stderr, "File being read is too short: %s\n", path);
    fclose(fp);
    return -1;
  }

  version = ntohl(version);

  bytes_read = fread(&count, 1, 4, fp);
  if (bytes_read != 4) {
    fprintf(stderr, "File being read is too short: %s\n", path);
    fclose(fp);
    return -1;
  }

  count = ntohl(count);

  if (count > 0) {
    entries_buffer = malloc(count * sizeof(struct Entry));
  } else {
    entries_buffer = NULL;
  }
  if (entries_buffer == NULL) {
    fprintf(stderr, "Out of memory reading index %s", path);
    fclose(fp);
    return -1;
  }

  out_index->entries = entries_buffer;
  out_index->count = count;
  out_index->capacity = count;

  for (size_t i = 0; i < count; i++) {
    struct Entry *e = &entries_buffer[i];

    uint16_t flags = 0;

    size_t name_len = 0;
    size_t entry_len = 0;
    size_t pad_len = 0;
    size_t bytes_read = 0;

    e->st.st_ctimespec.tv_sec = read_be32(fp);
    e->st.st_ctimespec.tv_nsec = read_be32(fp);
    e->st.st_mtimespec.tv_sec = read_be32(fp);
    e->st.st_mtimespec.tv_nsec = read_be32(fp);
    e->st.st_dev = read_be32(fp);
    e->st.st_ino = read_be32(fp);
    e->st.st_mode = read_be32(fp);
    e->st.st_uid = read_be32(fp);
    e->st.st_gid = read_be32(fp);
    e->st.st_size = read_be32(fp);

    fread(e->sha1, 1, 20, fp);

    flags = read_be16(fp);

    name_len = flags & 0xFFF;
    entry_len = 62 + name_len + 1;
    pad_len = 8 - (entry_len % 8);

    if (name_len > sizeof(e->path) - 1) {
      fprintf(stderr, "Path too long: %zu (max %zu)\n", name_len,
              sizeof(e->path) - 1);
      fclose(fp);
      free(entries_buffer);
      return -1;
    }
    bytes_read = fread(e->path, 1, name_len + 1, fp);
    if (bytes_read != name_len + 1) {
      fprintf(stderr, "Truncated index: %s\n", path);
      fclose(fp);
      free(entries_buffer);
      return -1;
    }
    fseek(fp, pad_len, SEEK_CUR);
  }

  fclose(fp);
  return 0;
}

void add_entry(const struct Entry in_entry, struct Index *out_index) {}
static void write_and_hash(const void *data, size_t len, FILE *fp,
                           SHA_CTX *ctx) {
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
    fprintf(stderr, "Could not open %s\n", path);
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
    write_and_hash(e->path, name_len + 1, fp, &ctx);
    write_and_hash(zeros, pad_len, fp, &ctx);
  }

  // add trailing 20 byte hash checksum

  SHA1_Final(digest, &ctx);
  fwrite(digest, 1, 20, fp);

  fclose(fp);
  return 0;
}

void free_index(struct Index *index) {
  if (index == NULL)
    return;
  free(index->entries);
  index->entries = NULL;
  index->count = 0;
  index->capacity = 0;
}