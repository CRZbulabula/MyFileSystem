#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "node.h"

void read_meta(struct fs_meta* meta, FILE* fd)
{
	fseek(fd, 0, SEEK_SET);
	fread((void *) meta, sizeof(struct fs_meta), 1, fd);
}

void save_meta(struct fs_meta* meta, FILE* fd)
{
	printf("save meta block: %d\n", meta->blcokUsed);
	fseek(fd, 0, SEEK_SET);
	fwrite((void *) meta, sizeof(struct fs_meta), 1, fd);
	fflush(fd);
	fsync(fd);
}

void read_inode(struct fs_inode *inode, off_t off, FILE* fd)
{
	fseek(fd, off * BLOCKSIZE, SEEK_SET);
	fread((void *) inode, sizeof(struct fs_inode), 1, fd);
	printf("read inode: %s Block: %d\n", inode->path, off * BLOCKSIZE);
}

void save_inode(struct fs_inode *inode, FILE* fd)
{
	//fseek(fd, 0, SEEK_END); //定位到文件末 
	//printf("file size %d\n", ftell(fd));
	//if(ftell(fd) == )
	printf("save inode: %s Block: %d\n", inode->path, inode->off * BLOCKSIZE);
	fseek(fd, inode->off * BLOCKSIZE, SEEK_SET);
	//printf("write size %d, stat size %d\n", sizeof(struct fs_inode), sizeof(struct stat));
	fwrite((void *) inode, sizeof(struct fs_inode), 1, fd);
	//fseek(fd, 0, SEEK_END); //定位到文件末 
	//printf("file size %d\n", ftell(fd));
	fflush(fd);
	fsync(fd);
}

void fs_init_inode(struct fs_inode* inode)
{
	bzero(inode->path, sizeof(inode->path));
	bzero(inode->childPath, sizeof(inode->childPath));
	memset(inode->childOff, -1, sizeof(inode->childOff));
	inode->off = inode->parent = -1;
}

struct fs_inode* fs_search_file(struct fs_meta *meta, char *path, FILE* fd)
{
	struct fs_inode* inode = &(meta->root);
	
	const char delim[2] = "/\0";
	char *subPath = strtok(path, delim);
	bool at_root = true;
	while (subPath != NULL) {
		printf("search path: %s\n", subPath);
		if (strcmp(subPath, "tmp") == 0 || strcmp(subPath, "disk") == 0) {
			subPath = strtok(NULL, delim);
			continue;
		}

		int i;
		bool searchFlag = false;
		for (i = 0; i < MAX_INODE; i++) {
			printf("child path: %s child off: %d\n", inode->childPath[i], inode->childOff[i]);
			if (strcmp(subPath, inode->childPath[i]) == 0) {
				struct fs_inode* next = (struct fs_inode *)malloc(sizeof(struct fs_inode));
				fs_init_inode(next);
				read_inode(next, inode->childOff[i], fd);
				printf("next: %s %d %s %d\n", next->path, next->off, next->childPath[0], next->childOff[0]);
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

void fs_create_file(struct fs_meta *meta, char *path, mode_t mode, FILE* fd)
{
	printf("%s: %s\n", __FUNCTION__, path);
	int i, last;
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
	struct fs_inode child;
	fs_init_inode(&child);
	for (i = 0; i < MAX_INODE; i++) {
		if (father->childOff[i] == -1) {
			strcpy(father->childPath[i], childPath);
			father->childOff[i] = meta->blcokUsed++;
			strcpy(child.path, childPath);
			child.off = father->childOff[i];
			child.vstat.st_mode = mode;

			if (father->off == -1) {
				save_meta(meta, fd);
			} else {
				save_inode(father, fd);
				save_meta(meta, fd);
			}
			save_inode(&child, fd);
			
			break;
		}
	}

	if (last) {
		free(father);
	}
}