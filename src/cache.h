#ifndef __CACHE__
#define __CACHE__

#include <fuse.h>

#include "def.h"

struct fs_cache {
    off_t prev, next, off;

    char data[MAX_DATA];
};

void read_cache(struct fs_cache* cache, off_t off, FILE* fd);
void save_cache(struct fs_cache* cache, FILE* fd);
void fs_init_cache(struct fs_cache* cache);

#endif