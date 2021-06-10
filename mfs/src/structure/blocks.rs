use super::{STAT_SIZE, STAT_VFS_SIZE, NODE_SIZE, NODE_NUM_TOTAL};

pub struct Superblock {
    pub padding: [u8;STAT_VFS_SIZE],
    pub inode_bitmap: u32,
    pub dnode_bitmap: u32,
    pub inode_begin: u32,
    pub dnode_begin: u32,
    pub inode_max: u32,
    pub dnode_max: u32
}

impl Superblock {
    pub fn new() -> Self {
        Self {
            padding: [0;STAT_VFS_SIZE],
            inode_bitmap: 1, //第1个block(superblock为第0个)
            dnode_bitmap: 2, //第2个block
            inode_begin: 3,
            dnode_begin: 512,
            inode_max: 511, //[3,511]，最多2MB
            dnode_max: (NODE_NUM_TOTAL-1) as u32 //[512,2047]，最后为8MB
        }
    }
}