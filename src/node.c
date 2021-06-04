#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "node.h"

void save_meta(struct fs_meta* meta, FILE* fd)
{
	printf("save meta block: %d\n", meta->blcokUsed);
	fseek(fd, 0, SEEK_SET);
	fwrite((void *) meta, sizeof(struct fs_meta), 1, fd);
}

void read_inode(struct fs_inode *inode, off_t off, FILE* fd)
{
	fseek(fd, off * BLOCKSIZE, SEEK_SET);
	fread((void *) inode, sizeof(struct fs_inode), 1, fd);
}

void save_inode(struct fs_inode *inode, FILE* fd)
{
	printf("save inode: %s\n", inode->path);
	fseek(fd, inode->off * BLOCKSIZE, SEEK_SET);
	fwrite((void *) inode, sizeof(struct fs_inode), 1, fd);
}

void fs_init_inode(struct fs_inode* inode)
{
	bzero(inode->path, sizeof(inode->path));
	bzero(inode->childPath, sizeof(inode->childPath));
	memset(inode->childOff, -1, sizeof(inode->childOff));
	inode->off = -1;
}

struct fs_inode* fs_search_file(struct fs_meta *meta, char *path, FILE* fd)
{
	struct fs_inode* inode = &(meta->root);
	const char delim[2] = "/\0";
	char *subPath = strtok(path, delim);
	while (subPath != NULL) {
		if (strcmp(subPath, "tmp") == 0 || strcmp(subPath, "disk") == 0) {
			subPath = strtok(NULL, delim);
			continue;
		}

		int i;
		bool searchFlag = false;
		for (i = 0; i < MAX_INODE; i++) {
			if (strcmp(subPath, inode->childPath[i]) == 0) {
				struct fs_inode next;
				fs_init_inode(&next);
				read_inode(&next, inode->childOff[i], fd);
				inode = &next;
				searchFlag = true;
			}
		}
		if (!searchFlag) {
			return -1;
		}

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
}