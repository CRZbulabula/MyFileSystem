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
	char padding[312];
};

// TODO: 所有类型的块都应该是4096Byte，不够的加padding如上
//这里fs_meta包含fs_inode，比较难办。好在现在超出4096的部分全在 fs_inode root 的 padding里，所以正确性不会出错
struct fs_meta {
	struct statvfs statv;
	//u32 blockUsed;//使用alloc/unalloc来分配块
	struct fs_inode root;
	//char padding[192];
};

void save_meta(struct fs_meta* meta, FILE* fd);
void read_inode(struct fs_inode* inode, off_t off, FILE* fd);
void save_inode(struct fs_inode* inode, FILE* fd);
void fs_init_inode(struct fs_inode* inode);
struct fs_inode* fs_search_file(struct fs_meta* meta, char* path, FILE* fd);
int fs_create_file(struct fs_meta* meta, char* path, mode_t mode, FILE* fd);

void fs_alloc_cache(struct fs_inode* inode, size_t new_size, FILE* fd);
struct fs_cache* fs_locate_cache(struct fs_inode* inode, off_t off, FILE* fd);
void fs_read_file(struct fs_cache* cache, char *buf, size_t size, off_t off, FILE* fd);
void fs_write_file(struct fs_cache* cache, const char *buf, size_t size, off_t off, FILE* fd);
void fs_delete_file(struct fs_inode* inode, FILE* fd);

void fs_update_time(struct fs_inode* inode, int which);

#endif