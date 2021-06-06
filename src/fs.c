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

/**
 * 列举当前目录下的所有文件
 */
static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
					  off_t offset, struct fuse_file_info* fi)
{
	printf("%s: %s\n", __FUNCTION__, path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (strcmp(path, "/") == 0) {
		// 根目录
		int i;
		for (i = 0; i < MAX_INODE; i++) {
			if (meta.root.childOff[i] != -1) {
				filler(buf, meta.root.childPath[i], NULL, 0);
			}
		}
	} else {
		struct fs_inode* inode = fs_search_file(&meta, path, fd);
		int i;
		for (i = 0; i < MAX_INODE; i++) {
			if (inode->childOff[i] != -1) {
				filler(buf, inode->childPath[i], NULL, 0);
			}
		}
	}

	return 0;
}

/**
 * 查询节点是否存在
 * 成功时会写入节点属性
 */
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
		// 路径不存在
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

/**
 * 进入目录
 */
static int fs_access(const char *path, int mask)
{
	int res = 0;
	printf("%s: %s\n", __FUNCTION__, path);

	struct fs_inode* inode = fs_search_file(&meta, path, fd);
	if (inode == -1) {
		// 路径不存在
		res = -2;
	}

	return res;
}

/**
 * 创建文件夹
 */
static int fs_mkdir(const char *path, mode_t mode)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *dirPath = malloc(strlen(path + 1));
	strcpy(dirPath, path);
	dirPath[strlen(path)] = '\0';
	fs_create_file(&meta, dirPath, S_IFDIR | 0755 | mode, fd);
	return 0;
}

/**
 * 删除目录
 */
static int fs_rmdir(const char *path)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *dirPath = malloc(strlen(path + 1));
	strcpy(dirPath, path);
	dirPath[strlen(path)] = '\0';
	
	struct fs_inode* inode = fs_search_file(&meta, path, fd);
	if (inode == -1) {
		// 路径不存在
		fclose(fd);
		return -2;
	}
	struct fs_inode father;
	read_inode(&father, inode->parent, fd);

	int i;
	for (i = 0; i < MAX_INODE; i++) {
		if (strcmp(father.childPath[i], inode->path) == 0) {
			bzero(father.childPath[i], sizeof(father.childPath[i]));
			father.childOff[i] = -1;
			break;
		}
	}
	save_inode(&father, fd);
	return 0;
}

/**
 * 创建文件
 */
static int fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';
	fs_create_file(&meta, filePath, S_IFREG | 0644 | mode, fd);
	return 0;
}

/**
 * 删除文件
 */
static int fs_unlink(const char *path)
{
	printf("%s: %s\n", __FUNCTION__, path);
	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';
	
	struct fs_inode* inode = fs_search_file(&meta, path, fd);
	if (inode == -1) {
		// 路径不存在
		return -2;
	}
	struct fs_inode father;
	read_inode(&father, inode->parent, fd);

	int i;
	for (i = 0; i < MAX_INODE; i++) {
		if (strcmp(father.childPath[i], inode->path) == 0) {
			bzero(father.childPath[i], sizeof(father.childPath[i]));
			father.childOff[i] = -1;
			break;
		}
	}
	save_inode(&father, fd);
	return 0;
}

static struct fuse_operations fs_ops = {
	.readdir = fs_readdir,
	.getattr = fs_getattr,
	.access = fs_access,

	.mknod = fs_mknod,
	.unlink = fs_unlink,

	.mkdir = fs_mkdir,
	.rmdir = fs_rmdir,
};

void init_fs_meta()
{
	printf("init meta\n");
	if (access("/tmp/data", F_OK) != 0) {
		printf("data file not exist\n");
		fd = fopen("/tmp/data", "wb+");
		printf("data file created\n");
		meta.blcokUsed = 2;
		fs_init_inode(&meta.root);
		save_meta(&meta, fd);
		return;
	}

	printf("data file exist\n");
	fd = fopen("/tmp/data", "rb+");
	fseek(fd, 0, SEEK_SET);
	fread((void *) &meta, sizeof(struct fs_meta), 1, fd);
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