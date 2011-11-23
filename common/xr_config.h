#ifndef XR_CONFIG_H
#define XR_CONFIG_H

#include <openssl/sha.h>

#define FILE_BLOCK_LINES 1000
#define TAG_BLOCK_LINES  100
#define HASH_WINDOW      10

static const char tag_pre[] = "t/";
static const char line_pre[] = "l/";
static const char src_pre[] = "s/";
static const char block_ext[] = ".xr";
#define OBJ_NAME_LEN \
   (SHA_DIGEST_LENGTH*2 + 1 + (sizeof(tag_pre)-1) + sizeof(block_ext))

#endif
