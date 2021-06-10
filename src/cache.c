#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cache.h"
#include "linker.h"

void read_cache(struct fs_cache *cache, off_t off, FILE* fd)
{
	//fseek(fd, off * BLOCKSIZE, SEEK_SET);
	//fread((void *) cache, sizeof(struct fs_cache), 1, fd);
	read_block(off, cache);
	//printf("read cache: %d Block: %d\n", cache->off * BLOCKSIZE, off * BLOCKSIZE);
}

void save_cache(struct fs_cache* cache, FILE* fd)
{
	//printf("save cache Block: %d\n", cache->off * BLOCKSIZE);
	//fseek(fd, cache->off * BLOCKSIZE, SEEK_SET);
	//fwrite((void *) cache, sizeof(struct fs_cache), 1, fd);
	write_block(cache->off, cache);
	//fflush(fd);
	//fsync(fd);
}

void fs_init_cache(struct fs_cache* cache)
{
	cache->prev = cache->next = cache->off = -1;
	bzero(cache->data, sizeof(cache->data));
}