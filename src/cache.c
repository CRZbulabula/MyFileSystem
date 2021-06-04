#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cache.h"

void read_cache(struct fs_cache *cache, off_t off, FILE* fd)
{
	fseek(fd, off * BLOCKSIZE, SEEK_SET);
	fread((void *) cache, sizeof(struct fs_cache), 1, fd);
}

void save_cache(struct fs_cache* cache, FILE* fd)
{
	fseek(fd, cache->off * BLOCKSIZE, SEEK_SET);
	fwrite((void *) cache, sizeof(struct fs_cache), 1, fd);
}