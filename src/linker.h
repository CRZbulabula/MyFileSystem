#ifndef _LINKER_H_
#define _LINKER_H_

#include <stdint.h>

//A+B
extern void test_(uint32_t a, uint32_t b);
//检查文件系统是否已存在。存在返回0，不存在返回-1。文件系统即 /src/file_system，可手动删除
extern int32_t check_file_system_existed();
//申请一个inode，失败(设备已满)返回-1
extern int32_t alloc_inode(); 
//释放一个inode，失败(本来就没有分配)返回-1。注意！！编号不是从0或1开始，请用alloc_inode返还的id
extern int32_t unalloc_inode(uint32_t node_id); 
//申请一个dnode(C代码这边叫cache)，失败返回-1
extern int32_t alloc_dnode(); 
//释放一个dnode，失败返回-1。注意同上
extern int32_t unalloc_dnode(uint32_t node_id); 
//写一个block(不会写盘上，直到fsync)。请用alloc_i/dnode返还的id。buf_ptr指向的一定是4096Byte，多了不会读/写，少了会出问题
extern void write_block(uint32_t block_id, const uint8_t* buf_ptr); 
//读一个block。请用alloc_i/dnode返还的id。buf_ptr指向的一定是4096Byte，多了不会读/写，少了会出问题
extern void read_block(uint32_t block_id, const uint8_t* buf_ptr); 
//同步所有更改的结点。重启后文件系统回停留在上次fsync后。
//除了用户调用fsync外，自己的fs.c中每次read/write/mkdir/rmdir...完成后也最好fsync。(当然不fsync不会影响正确性，只是结束后下次加载时文件系统是空的而已)
extern void fsync_rust();
//获取根节点的inode id。一般返回值都是3。
extern int32_t get_root_dir_inode_id();

void test_rust();

//void rs_read_block(uint32_t block_id, const uint8_t* buf_ptr);
//void rs_write_block(uint32_t block_id, const uint8_t* buf_ptr);

#endif
