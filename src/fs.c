#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <assert.h>
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
#include "linker.h"

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
	fsync_rust();
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
	
	struct fs_inode* inode = fs_search_file(&meta, dirPath, fd);
	if (inode == -1) {
		// 路径不存在
		return -2;
	}
	
	struct fs_inode* father = (struct fs_inode *) malloc(sizeof(struct fs_inode));
	if (inode->parent != -1) {
		read_inode(father, inode->parent, fd);
	} else {
		father = &(meta.root);
	}

	int i;
	for (i = 0; i < MAX_INODE; i++) {
		if (strcmp(father->childPath[i], inode->path) == 0) {
			bzero(father->childPath[i], sizeof(father->childPath[i]));
			father->childOff[i] = -1;
			break;
		}
	}

	if (inode->parent != -1) {
		save_inode(father, fd);
	} else {
		save_meta(&meta, fd);
	}
	fsync_rust();
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
	fs_create_file(&meta, filePath, S_IFREG | 0755 | mode, fd);
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
	
	struct fs_inode* inode = fs_search_file(&meta, filePath, fd);
	if (inode == -1) {
		// 路径不存在
		return -2;
	}
	struct fs_inode* father = (struct fs_inode *) malloc(sizeof(struct fs_inode));
	if (inode->parent != -1) {
		read_inode(father, inode->parent, fd);
	} else {
		father = &(meta.root);
	}

	int i;
	for (i = 0; i < MAX_INODE; i++) {
		if (strcmp(father->childPath[i], inode->path) == 0) {
			bzero(father->childPath[i], sizeof(father->childPath[i]));
			father->childOff[i] = -1;
			break;
		}
	}
	
	if (inode->parent != -1) {
		save_inode(father, fd);
	} else {
		save_meta(&meta, fd);
	}
	return 0;
}

/**
 * 重命名
 */
static int fs_rename(const char* path, const char* path_new)
{
	printf("%s: %s to: %s\n", __FUNCTION__, path, path_new);
	char* oldPath = malloc(strlen(path + 1));
	strcpy(oldPath, path);
	oldPath[strlen(path)] = '\0';

	int i, last;
	for (i = strlen(path) - 1; ; i--) {
		if (path[i] == '/') {
			last = i;
			break;
		}
	}
	char* oldName = (char*) malloc(strlen(path) - last);
	strncpy(oldName, path + last + 1, strlen(path) - last - 1);
	oldName[strlen(path) - last] = '\0';

	for (i = strlen(path_new) - 1; ; i--) {
		if (path_new[i] == '/') {
			last = i;
			break;
		}
	}
	char* newName = (char*) malloc(strlen(path_new) - last);
	strncpy(newName, path_new + last + 1, strlen(path_new) - last - 1);
	newName[strlen(path_new) - last] = '\0';

	struct fs_inode* inode = fs_search_file(&meta, oldPath, fd);
	if (inode == -1) {
		// 路径不存在
		return -2;
	}
	strcpy(inode->path, newName);

	printf("RENAME: %s %s\n", oldName, newName);
	struct fs_inode* father;
	if (inode->parent != -1) {
		father = (struct fs_inode*) malloc(sizeof(struct fs_inode));
		read_inode(father, inode->parent, fd);
		for (i = 0; i < MAX_INODE; i++) {
			if (strcmp(father->childPath[i], oldName) == 0) {
				puts("find and rename");
				strcpy(father->childPath[i], newName);
				break;
			}
		}
		save_inode(father, fd);
	} else {
		father = &(meta.root);
		for (i = 0; i < MAX_INODE; i++) {
			if (strcmp(father->childPath[i], oldName) == 0) {
				puts("find and rename");
				strcpy(father->childPath[i], newName);
				break;
			}
		}
		save_meta(&meta, fd);
	}
	save_inode(inode, fd);
	fsync_rust();
	return 0;
}

/**
 * 打开文件
 */
static int fs_open(const char *path, struct fuse_file_info *fi)
{
	printf("%s: %s\n", __FUNCTION__, path);
	int res = 0;
	
	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';
	
	struct fs_inode* inode = fs_search_file(&meta, filePath, fd);
	if (inode == -1) {
		if (fs_create_file(&meta, filePath, S_IFREG | 0755, fd) != 0) {
			// 路径不存在且无法创建
			return -2;
		}
		inode = fs_search_file(&meta, filePath, fd);
	} else if (inode->vstat.st_mode & S_IFDIR) {
		// 打开了一个目录
		return -21;
	}

	return res;
}

/**
 * 释放文件
 */
static int fs_release(const char *path, struct fuse_file_info *fi)
{
	printf("%s: %s\n", __FUNCTION__, path);
	return 0;
}

/**
 * 读取文件
 */
static int fs_read(const char *path,
					  char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
{
	printf("%s: %s size: %ld off: %lld\n", __FUNCTION__, path, size, offset);

	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';

	struct fs_inode* inode = fs_search_file(&meta, filePath, fd);
	if (inode == -1) {
		// 文件不存在
		return -2;
	}

	if (offset > inode->vstat.st_size) {
		return 0;
	}

	struct fs_cache* cache = (struct fs_cache*) malloc(sizeof(struct fs_cache));
	read_cache(cache, inode->data, fd);
	fs_locate_cache(inode, cache, offset, fd);

	if (offset + size > inode->vstat.st_size) {
		size = inode->vstat.st_size - offset;
	}

	char* resBuf = (char*) malloc(size + 1);
	resBuf[size + 1] = '\0';
	fs_read_file(cache, resBuf, size, offset, fd);
	printf("fs_read: %s, size: %ld, off: %lld, res: %s\n", path, size, offset, resBuf);

	strcpy(buf, resBuf);
	fsync_rust();
	return size;
}

/**
 * 写入文件
 */
static int fs_write(const char *path,
                       const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi)
{
	printf("%s: %s, buf: %s, size: %ld, off: %lld\n", __FUNCTION__, path, buf, size, offset);

	char *filePath = malloc(strlen(path + 1));
	strcpy(filePath, path);
	filePath[strlen(path)] = '\0';

	struct fs_inode* inode = fs_search_file(&meta, filePath, fd);
	if (inode == -1) {
		// 文件不存在
		return -2;
	}

	struct fs_cache* cache = (struct fs_cache*) malloc(sizeof(struct fs_cache));
	read_cache(cache, inode->data, fd);
	fs_locate_cache(inode, cache, offset, fd);
	fs_write_file(cache, buf, size, offset, fd);

	if (offset + size > inode->vstat.st_size) {
		inode->vstat.st_size = offset + size;
	}
	fs_update_time(inode, U_ALL);
	save_inode(inode, fd);
	fsync_rust();
	return size;
}

/* 下面三个函数看似无用但是绝对不能删！！！ */

/**
 * 重设文件大小
 */
static int fs_truncate(const char* path, off_t off, struct fuse_file_info* fi)
{
	return 0;
}

/**
 * 重设文件时间
 */
static int fs_utimens(const char* path, const struct timespec tv[2], struct fuse_file_info* fi)
{
	return 0;
}

/**
 * 重设文件权限
 */
static int fs_chown(const char* path, uid_t u, gid_t g, struct fuse_file_info* fi)
{
	return 0;
}

static struct fuse_operations fs_ops = {
	.readdir = fs_readdir,
	.getattr = fs_getattr,
	.access = fs_access,

	.mknod = fs_mknod,
	.unlink = fs_unlink,
	.rename = fs_rename,

	.open = fs_open,
	.release = fs_release,
	.read = fs_read,
	.write = fs_write,

	.mkdir = fs_mkdir,
	.rmdir = fs_rmdir,

	.truncate = fs_truncate,
	.utimens = fs_utimens,
	.chown = fs_chown,
};

void init_fs_meta()
{
	printf("init meta\n");
	//if (access("/tmp/disk/data", F_OK) != 0) {
	if(check_file_system_existed() == -1) {
		printf("data file not exist\n");
		/*
		fd = fopen("/tmp/data", "wb+");
		printf("data file created\n");
		meta.blockUsed = 2;
		fs_init_inode(&meta.root);

		//struct fs_meta* meta0 = (struct fs_meta *) malloc(sizeof(struct fs_meta));
		save_meta(&meta, fd);
		//save_meta(meta0, fd);
		*/
		fs_init_inode(&meta.root);
		int root_dir_inode_id = get_root_dir_inode_id();
		write_block(root_dir_inode_id, &meta); //文件系统不存在，需要创建
		fsync_rust(); //同步到盘上
		return;
		
	}

	printf("data file exist\n");
	int root_dir_inode_id = get_root_dir_inode_id();
	read_block(root_dir_inode_id, &meta); //文件系统已存在，读取即可
	/*
	fd = fopen("/tmp/disk/data", "rb+");
	fseek(fd, 0, SEEK_SET);
	fread((void *) &meta, sizeof(struct fs_meta), 1, fd);
	printf("blockUsed: %d\n", meta.blockUsed);
	*/
}

int main(int argc, char* argv[])
{
	//test_rust();
	printf("stat size: %ld\n", sizeof(struct stat));
	printf("statvfs size: %ld\n", sizeof(struct statvfs));
	printf("meta size: %ld\n", sizeof(struct fs_meta));
	printf("inode size: %ld\n", sizeof(struct fs_inode));
	printf("cache size: %ld\n", sizeof(struct fs_cache));
	//assert(sizeof(struct fs_meta) == BLOCKSIZE);//目前fs_meta由于包含fs_inode,稍微大于fs_inode，不过没有正确性问题
	assert(sizeof(struct fs_inode) == BLOCKSIZE);
	assert(sizeof(struct fs_cache) == BLOCKSIZE);
	init_fs_meta();

	int retCode = fuse_main(argc, argv, &fs_ops, NULL);
	return retCode;
}