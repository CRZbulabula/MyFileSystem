#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "node.h"
#include "linker.h"

void fs_update_time(struct fs_inode* inode, int which)
{
	time_t now = time(0);
	if (which & U_ATIME) {
		inode->vstat.st_atime = now;
	}
	if (which & U_CTIME) {
		inode->vstat.st_ctime = now;
	}
	if (which & U_MTIME) {
		inode->vstat.st_mtime = now;
	}
}

void read_meta(struct fs_meta* meta, FILE* fd)
{
	//fseek(fd, 0, SEEK_SET);
	//fread((void *) meta, sizeof(struct fs_meta), 1, fd);
	read_block(get_root_dir_inode_id(), (void*) &meta);
}

void save_meta(struct fs_meta* meta, FILE* fd)
{
	//((char*)meta)[0] = 1;
	//fseek(fd, 0, SEEK_SET);
	//fwrite((void *) meta, sizeof(struct fs_meta), 1, fd);
	//fflush(fd);
	write_block(get_root_dir_inode_id(), meta);
	//fsync(fd);
}

void read_inode(struct fs_inode *inode, off_t off, FILE* fd)
{
	//fseek(fd, off * BLOCKSIZE, SEEK_SET);
	//fread((void *) inode, sizeof(struct fs_inode), 1, fd);
	read_block(off, inode);
	printf("read inode: %s Block: %lld\n", inode->path, off * BLOCKSIZE);
}

void save_inode(struct fs_inode *inode, FILE* fd)
{
	//fseek(fd, 0, SEEK_END); //定位到文件末 
	//printf("file size %d\n", ftell(fd));
	//if(ftell(fd) == )
	printf("save inode: %s Block: %lld\n", inode->path, inode->off * BLOCKSIZE);
	//fseek(fd, inode->off * BLOCKSIZE, SEEK_SET);
	//printf("write size %d, stat size %d\n", sizeof(struct fs_inode), sizeof(struct stat));
	//fwrite((void *) inode, sizeof(struct fs_inode), 1, fd);
	write_block(inode->off, inode);
	//fseek(fd, 0, SEEK_END); //定位到文件末 
	//printf("file size %d\n", ftell(fd));
	//fflush(fd);
	//fsync(fd);
}

void fs_alloc_cache(struct fs_inode* inode, size_t new_size, FILE* fd)
{
	struct fs_cache* cache = (struct fs_cache*) malloc(sizeof(struct fs_cache));
	read_cache(cache, inode->data, fd);

	size_t cur_size = MAX_DATA;
	while (cur_size < new_size) {
		cur_size += MAX_DATA;
		struct fs_cache* next = (struct fs_cache*) malloc(sizeof(struct fs_cache));
		if (cache->next != -1) {
			read_cache(next, cache->next, fd);
		} else {
			int new_cache_off = alloc_dnode();
			cache->next = new_cache_off;

			fs_init_cache(next);
			next->prev = cache->off;
			next->off = new_cache_off;
			next->next = -1;

			save_cache(cache, fd);
			save_cache(next, fd);
		}
		free(cache);
		cache = next;
	}
}

struct fs_cache* fs_locate_cache(struct fs_inode* inode, off_t off, FILE* fd)
{
	struct fs_cache* cache = (struct fs_cache*) malloc(sizeof(struct fs_cache));
	read_cache(cache, inode->data, fd);

	int cur_id = 0;
	int start_id = off / MAX_DATA;
	while (cur_id < start_id) {
		struct fs_cache* next = (struct fs_cache*) malloc(sizeof(struct fs_cache));
		read_cache(next, cache->next, fd);
		free(cache);
		cache = next;
		cur_id++;
	}

	return cache;
}

void fs_read_file(struct fs_cache* cache, char *buf, size_t size, off_t off, FILE* fd)
{
	off_t off_sum = 0;
	off_t start_off = off % MAX_DATA;
	size_t size_res = size;
	while (size_res) {
		size_t size_read = MAX_DATA - start_off;
		if (size_read > size_res) {
			size_read = size_res;
		}

		strncpy(buf + off_sum, cache->data + start_off, size_read);
		size_res -= size_read;
		off_sum += size_read;
		start_off = 0;

		if (!size_res) {
			break;
		}

		struct fs_cache* next = (struct fs_cache*) malloc(sizeof(struct fs_cache));
		read_cache(next, cache->next, fd);
		free(cache);
		cache = next;
	}
}

void fs_write_file(struct fs_cache* cache, const char *buf, size_t size, off_t off, FILE* fd)
{
	int write_block_cnt = 0;
	off_t off_sum = 0;
	off_t start_off = off % MAX_DATA;
	size_t size_res = size;
	while (size_res) {
		size_t size_fill = MAX_DATA - start_off;
		if (size_fill > size_res) {
			size_fill = size_res;
		}

		strncpy(cache->data + start_off, buf + off_sum, size_fill);
		size_res -= size_fill;
		off_sum += size_fill;
		start_off = 0;
		write_block_cnt++;
		save_cache(cache, fd);

		if (!size_res) {
			break;
		}

		struct fs_cache* next = (struct fs_cache*) malloc(sizeof(struct fs_cache));
		read_cache(next, cache->next, fd);
		free(cache);
		cache = next;
	}
}

void fs_delete_file(struct fs_inode* inode, FILE* fd)
{
	int i;

	if (inode->data != -1) {
		struct fs_cache* cache = (struct fs_cache*) malloc(sizeof(struct fs_cache));
		read_cache(cache, inode->data, fd);
		while (1) {
			printf("free cache: %ld %ld\n", cache->off, cache->next);
			unalloc_dnode(cache->off);
			if (cache->next == -1) {
				break;
			}

			struct fs_cache* next = (struct fs_cache*) malloc(sizeof(struct fs_cache));
			read_cache(next, cache->next, fd);
			free(cache);
			cache = next;
		}
	}

	for (i = 0; i < MAX_INODE; i++) {
		if (inode->childOff[i] != -1) {
			struct fs_inode* next = (struct fs_inode*) malloc(sizeof(struct fs_inode));
			fs_init_inode(next);
			fs_delete_file(next, fd);
		}
	}

	printf("free inode: %s", inode->path);
	unalloc_inode(inode->off);
}

void fs_init_inode(struct fs_inode* inode)
{
	bzero(inode->path, sizeof(inode->path));
	bzero(inode->childPath, sizeof(inode->childPath));
	memset(inode->childOff, -1, sizeof(inode->childOff));
	inode->off = inode->parent = inode->data = -1;
}

struct fs_inode* fs_search_file(struct fs_meta *meta, char *path, FILE* fd)
{
	struct fs_inode* inode = &(meta->root);
	
	const char delim[2] = "/\0";
	char *subPath = strtok(path, delim);
	bool at_root = true;
	while (subPath != NULL) {
		printf("search path: %s\n", subPath);
		/*
		if (strcmp(subPath, "tmp") == 0 || strcmp(subPath, "disk") == 0) {
			subPath = strtok(NULL, delim);
			continue;
		}
		*/
		int i;
		bool searchFlag = false;
		for (i = 0; i < MAX_INODE; i++) {
			//printf("child path: %s child off: %d\n", inode->childPath[i], inode->childOff[i]);
			if (strcmp(subPath, inode->childPath[i]) == 0) {
				struct fs_inode* next = (struct fs_inode *)malloc(sizeof(struct fs_inode));
				fs_init_inode(next);
				read_inode(next, inode->childOff[i], fd);
				//printf("next: %s %d %d\n", next->path, next->off, next->parent);
				if (!at_root)
					free(inode);
				inode = next;
				searchFlag = true;
				break;
			}
		}
		if (!searchFlag) {
			return -1;
		}
		at_root = false;
		subPath = strtok(NULL, delim);
	}

	return inode;
}

int fs_create_file(struct fs_meta *meta, char *path, mode_t mode, FILE* fd)
{
	printf("%s: %s\n", __FUNCTION__, path);
	int i, last, j;
	for (i = strlen(path) - 1; i >= 0; i--) {
		if (path[i] == '/') {
			last = i;
			break;
		}
	}
	char fatherPath[last + 1];
	if (last) {
		strncpy(fatherPath, path, last);
	}
	fatherPath[last] = '\0';
	char childPath[strlen(path) - last];
	strncpy(childPath, path + last + 1, strlen(path) - last - 1);
	childPath[strlen(path) - last - 1] = '\0';
	printf("fatherPath: %s childPath: %s\n", fatherPath, childPath);

	struct fs_inode* father;
	if (last) {
		father = fs_search_file(meta, fatherPath, fd);
	} else {
		father = &(meta->root);
	}
	if (father == -1) {
		return -1;
	}
	struct fs_inode child;
	fs_init_inode(&child);
	for (i = 0; i < MAX_INODE; i++) {
		if (father->childOff[i] == -1) {
			strcpy(father->childPath[i], childPath);
			father->childOff[i] = alloc_inode();
			printf("new child off = %d\n", father->childOff[i]);
			strcpy(child.path, childPath);
			
			child.parent = father->off;
			child.off = father->childOff[i];
			child.vstat.st_mode = mode;
			child.vstat.st_size = 0;

			if (mode & S_IFREG) {
				int alloced_now = alloc_dnode();
				int alloced_last = -1;
				child.data = alloced_now;
				for (j = 0; j < DATA_BLOCK_SET; j++) {
					struct fs_cache cache;
					fs_init_cache(&cache);
					if (j) {
						cache.prev = alloced_last;
					} else {
						cache.prev = -1;
					}
					if (j < DATA_BLOCK_SET - 1) {
						cache.next = alloced_now + 1; //TODO:分配不一定连续(硬盘紧张时)，所以这里之后要改
						//不过应该是整段改成“不要创建文件时分配一堆block”，所以其实也不用单独改上面这句
					} else {
						cache.next = -1;
					}
					cache.off = alloced_now;
					alloced_last = alloced_now;
					if (j < DATA_BLOCK_SET - 1) {
						alloced_now = alloc_dnode();
					}
					printf("%ld %ld\n", cache.off, cache.next);
					save_cache(&cache, fd);
				}
			}

			if (father->off == -1) {
				save_meta(meta, fd);
			} else {
				save_inode(father, fd);
				save_meta(meta, fd);
			}
			fs_update_time(&child, U_ALL);
			save_inode(&child, fd);
			
			break;
		}
	}

	if (last) {
		free(father);
	}

	return 0;
}