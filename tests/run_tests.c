#include "../include/commit.h"
#include "../include/index.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0, tests_failed = 0;

#define CHECK(name, cond)                                                      \
  do {                                                                         \
    tests_run++;                                                               \
    if (cond) {                                                                \
      printf("  ok   %s\n", name);                                             \
    } else {                                                                   \
      tests_failed++;                                                          \
      printf("  FAIL %s  (%s:%d)\n", name, __FILE__, __LINE__);                \
    }                                                                          \
  } while (0)

static void test_tree_row(void) {
  // A known entry: name "x", and a known 20-byte sha.
  // Use x's real blob sha so this matches what git would emit.
  unsigned char sha[20] = {0x78, 0x98, 0x19, 0x22, 0x61, 0x3b, 0x2a,
                           0xfb, 0x60, 0x25, 0x04, 0x2f, 0xf6, 0xbd,
                           0x87, 0x8a, 0xc1, 0x99, 0x4e, 0x85};
  const char *name = "x";

  // Build the row: "100644 x\0" + 20 raw sha bytes
  unsigned char actual[300];
  int n = sprintf((char *)actual, "100644 %s",
                  name); // writes "100644 x", NUL at actual[n]
  memcpy(actual + n + 1, sha, 20);
  size_t actual_len = (size_t)n + 1 + 20;

  // Expected bytes, hand-written from git's tree dump.
  unsigned char expected[] = {
      0x31, 0x30, 0x30, 0x36, 0x34, 0x34, // "100644"
      0x20,                               // space
      0x78,                               // "x"
      0x00,                               // NUL
      0x78, 0x98, 0x19, 0x22, 0x61, 0x3b, 0x2a, 0xfb, 0x60, 0x25,
      0x04, 0x2f, 0xf6, 0xbd, 0x87, 0x8a, 0xc1, 0x99, 0x4e, 0x85 // 20 sha bytes
  };
  size_t expected_len = sizeof(expected);

  CHECK("tree row length", actual_len == expected_len);
  CHECK("tree row bytes", actual_len == expected_len &&
                              memcmp(actual, expected, expected_len) == 0);
}

static void test_write_tree_sha(void) {
  // Build the index in memory: x, y, z with their known blob shas.
  // These are the exact shas from your verified index dumps.
  struct Entry entries[3];
  memset(entries, 0, sizeof(entries));

  unsigned char sha_x[20] = {0x78, 0x98, 0x19, 0x22, 0x61, 0x3b, 0x2a,
                             0xfb, 0x60, 0x25, 0x04, 0x2f, 0xf6, 0xbd,
                             0x87, 0x8a, 0xc1, 0x99, 0x4e, 0x85};
  unsigned char sha_y[20] = {0x61, 0x78, 0x07, 0x98, 0x22, 0x8d, 0x17,
                             0xaf, 0x2d, 0x34, 0xfc, 0xe4, 0xcf, 0xbd,
                             0xf3, 0x55, 0x56, 0x83, 0x24, 0x72};
  unsigned char sha_z[20] = {0xf2, 0xad, 0x6c, 0x76, 0xf0, 0x11, 0x5a,
                             0x6b, 0xa5, 0xb0, 0x04, 0x56, 0xa8, 0x49,
                             0x81, 0x0e, 0x7e, 0xc0, 0xaf, 0x20};

  strcpy(entries[0].path, "x");
  memcpy(entries[0].sha1, sha_x, 20);
  strcpy(entries[1].path, "y");
  memcpy(entries[1].sha1, sha_y, 20);
  strcpy(entries[2].path, "z");
  memcpy(entries[2].sha1, sha_z, 20);

  struct Index idx = {.entries = entries, .count = 3, .capacity = 3};

  char out_hex[41];
  unsigned char out_raw[20];
  int rc = write_tree(&idx, out_hex, out_raw);

  CHECK("write_tree returns 0", rc == 0);
  CHECK("tree sha matches git",
        strcmp(out_hex, "1a3eb013a13fbe197effac28e9f6baffff317dc4") == 0);
}

static void test_commit_format(void) {
  char buf[4096];
  time_t fixed_ts = 1780929211;
  int len = build_commit_buffer("1a3eb013a13fbe197effac28e9f6baffff317dc4",
                                NULL, "first", fixed_ts, buf, sizeof(buf));

  CHECK("commit len positive", len > 0);
  CHECK("has tree line",
        strstr(buf, "tree 1a3eb013a13fbe197effac28e9f6baffff317dc4\n") != NULL);
  CHECK("has author line", strstr(buf, "author ") != NULL);
  CHECK("has committer line", strstr(buf, "committer ") != NULL);
  CHECK("blank line before message", strstr(buf, "+0000\n\nfirst\n") != NULL);
  CHECK("no parent line for first commit", strstr(buf, "parent ") == NULL);
}

int main(void) {
  test_tree_row();
  test_write_tree_sha();
  test_commit_format();
  printf("\n%d run, %d failed\n", tests_run, tests_failed);
  return tests_failed ? 1 : 0;
}
