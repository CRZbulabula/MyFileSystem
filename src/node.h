#ifndef __NODE__
#define __NODE__

#include <fuse.h>
#include <pthread.h>
#include <stdlib.h>

#include "def.h"
#include "cache.h"

struct fs_inode {
	off_t off, parent, data;

	char path[MAX_NAME];
	char childPath[MAX_INODE][MAX_NAME];
	off_t childOff[MAX_INODE];

	struct stat vstat;
};

struct fs_meta {
	struct fs_inode root;
	u32 blcokUsed;

	struct statvfs statv;
};

void save_meta(struct fs_meta* meta, FILE* fd);
void read_inode(struct fs_inode* inode, off_t off, FILE* fd);
void save_inode(struct fs_inode* inode, FILE* fd);
void fs_init_inode(struct fs_inode* inode);
struct fs_inode* fs_search_file(struct fs_meta* meta, char* path, FILE* fd);
int fs_create_file(struct fs_meta* meta, char* path, mode_t mode, FILE* fd);

void fs_locate_cache(struct fs_inode* inode, struct fs_cache* cache, off_t off, FILE* fd);
void fs_read_file(struct fs_cache* cache, char *buf, size_t size, off_t off, FILE* fd);
void fs_write_file(struct fs_cache* cache, const char *buf, size_t size, off_t off, FILE* fd);

void fs_update_time(struct fs_inode* inode, int which);

#endif