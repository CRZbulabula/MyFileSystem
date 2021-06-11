use super::{STAT_SIZE, STAT_VFS_SIZE, NODE_SIZE, NODE_NUM_TOTAL, NODE_FOR_RECOVERY};
use std::mem::size_of;

#[derive(Copy, Clone)] //方便存取时类型转换
pub struct Superblock {
    //pub padding: [u8;STAT_VFS_SIZE],
    pub inode_bitmap: u32,
    pub dnode_bitmap: u32,
    pub inode_begin: u32,
    pub dnode_begin: u32,
    pub inode_max: u32,
    pub dnode_max: u32,
    pub root_dir_inode: u32,
    padding: [u8;NODE_SIZE - size_of::<u32>() * 7], //填充满node size
}

impl Superblock {
    pub fn new() -> Self {
        Self {
            //padding: [0;STAT_VFS_SIZE],
            inode_bitmap: 1, //第1个block(superblock为第0个)
            dnode_bitmap: 2, //第2个block
            inode_begin: 3,
            dnode_begin: 512,
            inode_max: 511, //[3,511]，最多2MB
            dnode_max: (NODE_NUM_TOTAL-1) as u32, //[512,16383]，最后为8MB
            root_dir_inode: 3,
            padding: [0u8;NODE_SIZE - size_of::<u32>() * 7],
        }
    }
}

#[derive(Copy, Clone)] //方便存取时类型转换
pub struct InodeBitmap {
    pub allocated: [u8;NODE_SIZE],
}

impl InodeBitmap {
    pub fn new() -> Self {
        Self {
            allocated: [0u8;NODE_SIZE],
        }
    }

    pub fn alloc(&mut self, max_size:usize) -> isize {
        let mut id:isize = -1;
        for i in 0..=(max_size / 8) {
            if self.allocated[i] != 0xff {
                for j in 0..8 {
                    if (self.allocated[i] & (1<<j)) == 0 {
                        let alloc_id = i * 8 + j;
                        if alloc_id < max_size {
                            id = alloc_id as isize;
                            self.allocated[i] |= 1<<j;
                        }
                        return id
                    }
                }
            }
        };
        id
    }
    pub fn unalloc(&mut self, inode_id:usize) -> isize {
        let byte = inode_id/8;
        let bit = 1<<(inode_id % 8);
        if (self.allocated[byte] & bit) == 0 { //没有分配这一块
            -1
        } else {
            self.allocated[byte] ^= bit;
            0
        }
    }
    pub fn test_used(&mut self, max_size:usize) {
        let mut cnt = 0;
        for i in 0..=(max_size / 8) {
            for j in 0..8 {
                let alloc_id = i * 8 + j;
                if alloc_id > max_size {
                    break
                }
                if (self.allocated[i] & (1<<j)) != 0 {
                    cnt += 1
                }
            };
        };    
        println!("[rust] inode used: {} / {}", cnt, max_size);
    }
}

#[derive(Copy, Clone)] //方便存取时类型转换
pub struct DnodeBitmap {
    pub allocated: [u8;NODE_SIZE],
}

impl DnodeBitmap {
    pub fn new() -> Self {
        Self {
            allocated: [0u8;NODE_SIZE],
        }
    }

    pub fn alloc_normal(&mut self, max_size:usize) -> isize {
        let mut id:isize = -1;
        for i in 0..=(max_size / 8) {
            if self.allocated[i] != 0xff {
                for j in 0..8 {
                    if (self.allocated[i] & (1<<j)) == 0 { 
                        let alloc_id = i * 8 + j;
                        if alloc_id < max_size {
                            id = alloc_id as isize;
                            self.allocated[i] |= 1<<j;
                        }
                        return id
                    }
                }
            }
        };
        id
    }
    pub fn alloc_log(&mut self, begin_size:usize, max_size:usize) -> isize {
        let mut id:isize = -1;
        for i in (begin_size / 8)..=(max_size / 8) {
            if self.allocated[i] != 0xff {
                for j in 0..8 {
                    if (self.allocated[i] & (1<<j)) == 0 {
                        let alloc_id = i * 8 + j;
                        if alloc_id < begin_size {
                            continue
                        }
                        else if alloc_id < max_size {
                            id = alloc_id as isize;
                            self.allocated[i] |= 1<<j;
                        }
                        return id
                    }
                }
            }
        };
        id
    }
    pub fn unalloc(&mut self, inode_id:usize) -> isize {
        let byte = inode_id/8;
        let bit = 1<<(inode_id % 8);
        if (self.allocated[byte] & bit) == 0 { //没有分配这一块
            -1
        } else {
            self.allocated[byte] ^= bit;
            0
        }
    }
    pub fn test_used(&mut self, max_size:usize) {
        let mut cnt = 0;
        for i in 0..=(max_size / 8) {
            for j in 0..8 {
                let alloc_id = i * 8 + j;
                if alloc_id > max_size {
                    break
                }
                if (self.allocated[i] & (1<<j)) != 0 {
                    cnt += 1
                }
            };
        };    
        println!("[rust] dnode used: {} / {}", cnt, max_size);
    }
}

#[derive(Copy, Clone)] //方便存取时类型转换
pub struct NodeCache {
    pub data: [u8;NODE_SIZE],
}

impl NodeCache {
    pub fn new() -> Self {
        Self {
            data: [0u8;NODE_SIZE],
        }
    }
}