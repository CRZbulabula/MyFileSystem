#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <fuse.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "node.h"
#include "cache.h"

static struct fs_meta meta;
static FILE* fd;

static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
					  off_t offset, struct fuse_file_info* fi)
{
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	int i;
	for (i = 0; i < MAX_INODE; i++) {
		if (meta.root.childOff[i] != -1) {
			filler(buf, meta.root.childPath[i], NULL, 0);
		}
	}

	return 0;
}

static int fs_getattr(const char* path, struct stat* st)
{
	int res = 0;
	printf("%s: %s\n", __FUNCTION__, path);
	memset(st, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		st->st_mode = 0755 | S_IFDIR;
		return 0;
	}

	struct fs_inode* inode = fs_search_file(&meta, path, fd);
	if (inode == -1) {
		res = -2;
	} else {
		*st = inode->vstat;
	}
	

	/*if (strcmp(path, "/") == 0)
		st->st_mode = 0755 | S_IFDIR;
	else
		st->st_mode = 0644 | S_IFREG;*/

	return res;
}


static int fs_mkdir(const char *path, mode_t mode)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *dirPath = malloc(strlen(path + 1));
	strcpy(dirPath, path);
	dirPath[strlen(path)] = '\0';
	fs_create_file(&meta, dirPath, S_IFDIR | mode, fd);
	return 0;
}

static int fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';
	fs_create_file(&meta, filePath, S_IFREG | mode, fd);
	return 0;
}

static struct fuse_operations fs_ops = {
	.readdir = fs_readdir,
	.getattr = fs_getattr,

	.mknod = fs_mknod,
	.mkdir = fs_mkdir,
};

void init_fs_meta()
{
	printf("init meta\n");
	if (access("/tmp/disk/data", F_OK) != 0) {
		printf("data file not exist\n");
		fd = fopen("/tmp/disk/data", "wb+");
		printf("data file created\n");
		meta.blcokUsed = 2;
		fs_init_inode(&meta.root);
		save_meta(&meta, fd);
		return;
	}

	printf("data file exist\n");
	fd = fopen("/tmp/disk/data", "rb+");
	fseek(fd, 0, SEEK_SET);
	fread((void *) &meta, sizeof(struct fs_meta), 1, fd);
	fclose(fd);
	printf("blockUsed: %d\n", meta.blcokUsed);
}

int main(int argc, char* argv[])
{
	printf("meta size: %ld\n", sizeof(struct fs_meta));
	printf("inode size: %ld\n", sizeof(struct fs_inode));
	init_fs_meta();

	int retCode = fuse_main(argc, argv, &fs_ops, NULL);
	return retCode;
}