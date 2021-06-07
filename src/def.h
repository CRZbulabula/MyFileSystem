#ifndef __DEF__
#define __DEF__

#define BLOCKSIZE   (1024UL * 4)

#define MAX_NAME    16
#define MAX_INODE   150
#define MAX_DATA    4000
#define DATA_BLOCK_SET 2560
#define MAX_BLOCKS  (1024UL * 1024)

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long
#define off_t unsigned long long

#define U_ATIME (1 << 0)
#define U_CTIME (1 << 1)
#define U_MTIME (1 << 2)
#define U_ALL   (U_ATIME | U_CTIME | U_MTIME)

#endif