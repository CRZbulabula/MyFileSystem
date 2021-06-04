#ifndef __NODE__
#define __NODE__

#include <fuse.h>
#include <pthread.h>

#include "def.h"

struct fs_inode {
	off_t off;

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
void fs_init_inode(struct fs_inode* inode);
struct fs_inode* fs_search_file(struct fs_meta *meta, char *path, FILE* fd);
void fs_create_file(struct fs_meta *meta, char *path, mode_t mode, FILE* fd);

#endif