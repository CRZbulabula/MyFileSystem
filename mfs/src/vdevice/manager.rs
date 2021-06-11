
use lazy_static::*;
use std::collections::{HashSet, HashMap};
use std::mem::size_of;
use std::fs::{File};
use std::sync::Mutex;
use std::slice;

use super::{read_block_raw, write_block_raw};
use super::FILE_SYSTEM_PATH;
use crate::structure::{Superblock, InodeBitmap, DnodeBitmap, NodeCache, NODE_SIZE, NODE_FOR_RECOVERY, NODE_NUM_TOTAL};

lazy_static! {
    pub static ref DEVICE_MANAGER:Mutex<DeviceManager> = Mutex::new({
        let mut dm = DeviceManager::new();
        dm.recover(); //启动时尝试查找log并恢复
        dm
    });
}

pub struct DeviceManager {
    superblock:Superblock,
    inode_bitmap:InodeBitmap,
    dnode_bitmap:DnodeBitmap,
    cached: HashMap<u32, NodeCache>,
    dirty: HashSet<u32>,
}

impl DeviceManager {
    pub fn new() -> Self {
        println!("[rvd] start!");
        assert_eq!(size_of::<Superblock>(), NODE_SIZE);
        assert_eq!(size_of::<InodeBitmap>(), NODE_SIZE);
        assert_eq!(size_of::<DnodeBitmap>(), NODE_SIZE);
        let mut superb = Superblock::new();
        let mut imap = InodeBitmap::new();
        let mut dmap = DnodeBitmap::new();

        let file_existed = {
            if let Ok(_f) = File::open(FILE_SYSTEM_PATH) {
                true
            } else {
                false
            }
        };
        if file_existed { //如果原有fs还在
            let mut buf = [0u8;NODE_SIZE];
            read_block_raw(0, &mut buf).unwrap(); //读super block
            unsafe {
                superb = *(&buf as * const _ as * const Superblock);
            };
            read_block_raw(1, &mut buf).unwrap(); //读imap
            unsafe {
                imap = *(&buf as * const _ as * const InodeBitmap);
            };
            read_block_raw(2, &mut buf).unwrap(); //读dmap
            unsafe {
                dmap = *(&buf as * const _ as * const DnodeBitmap);
            };
        } else { //否则，写入当前初始化的元信息
            unsafe {
                write_block_raw(0, slice::from_raw_parts_mut(&superb as * const _ as *mut u8, NODE_SIZE)).unwrap();
                write_block_raw(1, slice::from_raw_parts_mut(&imap as * const _ as *mut u8, NODE_SIZE)).unwrap();
                write_block_raw(2, slice::from_raw_parts_mut(&dmap as * const _ as *mut u8, NODE_SIZE)).unwrap();
            }
        }
        imap.test_used((superb.inode_max - superb.inode_begin + 1) as usize);
        dmap.test_used((superb.dnode_max - superb.dnode_begin + 1) as usize);
        Self {
            superblock: superb,
            inode_bitmap: imap,
            dnode_bitmap: dmap,
            cached: HashMap::new(),
            dirty: HashSet::new(),
        }
    }

    fn alloc_inode(&mut self) -> isize {
        let sblk = &self.superblock;
        let id = self.inode_bitmap.alloc((sblk.inode_max - sblk.inode_begin + 1) as usize);
        if id == -1 {
            -1 //空间已满
        } else {
            self.dirty.insert(sblk.inode_bitmap);
            println!("[rvd] alloc inode {}", id + sblk.inode_begin as isize);
            id + sblk.inode_begin as isize
        }
    }

    fn unalloc_inode(&mut self, node_id:u32) -> isize {
        self.dirty.insert(node_id);
        self.dirty.insert(self.superblock.inode_bitmap);
        println!("[rvd] unalloc inode {}", node_id);
        self.inode_bitmap.unalloc(node_id as usize)
    }

    fn alloc_dnode(&mut self) -> isize {
        let sblk = &self.superblock;
        let id = self.dnode_bitmap.alloc_normal((sblk.dnode_max - sblk.dnode_begin + 1) as usize);
        if id == -1 {
            -1 //空间已满
        } else {
            self.dirty.insert(sblk.dnode_bitmap);
            println!("[rvd] alloc dnode {}", id + sblk.inode_begin as isize);
            id + sblk.dnode_begin as isize
        }
    }

    fn unalloc_dnode(&mut self, node_id:u32) -> isize {
        self.dirty.insert(node_id);
        self.dirty.insert(self.superblock.dnode_bitmap);
        println!("[rvd] unalloc dnode {}", node_id);
        self.dnode_bitmap.unalloc(node_id as usize)
    }

    fn read_block_safe(&mut self, block_id:usize, buf: &mut [u8]) {
        println!("[rvd] read node {}", block_id);
        if block_id <= 2 {
            let buf_s = if block_id == 0 {
                unsafe {
                    slice::from_raw_parts(&self.superblock as * const _ as * const u8, NODE_SIZE)
                }
            } else if block_id == 1 {
                unsafe {
                    slice::from_raw_parts(&self.inode_bitmap as * const _ as * const u8, NODE_SIZE)
                }
            } else {
                unsafe {
                    slice::from_raw_parts(&self.dnode_bitmap as * const _ as * const u8, NODE_SIZE)
                }
            };
            for i in 0..NODE_SIZE {
                buf[i] = buf_s[i];
            }
        } else {
            if let Some(node) = self.cached.get(&(block_id as u32)) { //这里只获取cache的指针，开销比较小
                for i in 0..NODE_SIZE {
                    buf[i] = node.data[i];
                }
            } else { //否则，从文件爬取
                let mut new_cache_node = NodeCache::new();
                read_block_raw(block_id, &mut new_cache_node.data).unwrap();
                for i in 0..NODE_SIZE {
                    buf[i] = new_cache_node.data[i];
                }
                self.cached.insert(block_id as u32, new_cache_node); //插入cached，此时不需要处理dirty，因为和盘上是一致的
            }
        }
    }

    fn write_block_safe(&mut self, block_id:usize, buf: &mut [u8]) { //这里并不真正写入
        println!("[rvd] write node {}", block_id);
        self.dirty.insert(block_id as u32); //修改这一页，标记为dirty
        if let Some(node) = self.cached.get_mut(&(block_id as u32)) { //如果已缓存
            for i in 0..NODE_SIZE {
                node.data[i] = buf[i];
            }
        } else {
            let mut new_cache_node = NodeCache::new();
            for i in 0..NODE_SIZE {
                new_cache_node.data[i] = buf[i];
            }
            self.cached.insert(block_id as u32, new_cache_node);
        }
    }

    fn fsync(&mut self) {
        println!("[rvd] fsync start");
        assert_eq!(self.superblock.log_size, 0);
        let logged = self.dirty.len() < NODE_FOR_RECOVERY / 4;
        for cached_node in self.dirty.iter() {
            if logged {
                let log_node_at = NODE_NUM_TOTAL - NODE_FOR_RECOVERY + self.superblock.log_size as usize;
                self.superblock.log_node_id[self.superblock.log_size as usize] = *cached_node;
                let mut orig_data = [0u8;NODE_SIZE];
                read_block_raw(*cached_node as usize, &mut orig_data).unwrap();
                unsafe {
                *((&orig_data[NODE_SIZE - 4])  as *const _ as *mut u32) = *cached_node;
                }
                write_block_raw(log_node_at, &mut orig_data).unwrap();
                println!("[rvd] node {} logged", *cached_node);
                self.superblock.log_size += 1;
            }
            println!("[rvd] fsync update disk node {}", cached_node);
            if cached_node <= &2 { //是superblock / bitset
                let mut buf_s = if cached_node == &0 {
                    unsafe {
                        slice::from_raw_parts_mut(&self.superblock as * const _ as * mut u8, NODE_SIZE)
                    }
                } else if cached_node == &1 {
                    unsafe {
                        slice::from_raw_parts_mut(&self.inode_bitmap as * const _ as * mut u8, NODE_SIZE)
                    }
                } else {
                    unsafe {
                        slice::from_raw_parts_mut(&self.dnode_bitmap as * const _ as * mut u8, NODE_SIZE)
                    }
                };
                write_block_raw(*cached_node as usize, &mut buf_s).unwrap();
            } else {
                if let Some(node) = self.cached.get_mut(&cached_node) { //如果已经在cached里
                    write_block_raw(*cached_node as usize, &mut node.data).unwrap();
                } else { //不在cached里，说明是unalloc导致的，为了安全也要刷新旧数据
                    let mut buf = [0u8;NODE_SIZE];
                    write_block_raw(*cached_node as usize, &mut buf).unwrap();
                }
            }
        }
        self.dirty.clear(); //清空脏页
        if logged {
            let log_node_at = NODE_NUM_TOTAL - NODE_FOR_RECOVERY;
            self.superblock.log_size = 0;
            let mut orig_data = [0u8;NODE_SIZE];
            for i in 0..NODE_SIZE  {
                orig_data[i] = MAGIC_BARRIER[i % 4];
            };
            write_block_raw(log_node_at, &mut orig_data).unwrap();
            println!("[rvd] log committed.");
        }
        println!("[rvd] fsync end");
    }

    fn get_root_dir_inode_id(&mut self) -> isize {
        if (self.inode_bitmap.allocated[0] & 1) == 0 {
            self.alloc_inode()
        } else {
            self.superblock.inode_begin as isize
        }
    }

    fn recover(&mut self) -> i32 {
        println!("[rvd] recover start");
        let log_node_at = NODE_NUM_TOTAL - NODE_FOR_RECOVERY;
        let mut orig_data = [0u8;NODE_SIZE];
        read_block_raw(log_node_at, &mut orig_data).unwrap();
        let mut log_exist = false;
        for i in 0..NODE_SIZE {
            if orig_data[i] != MAGIC_BARRIER[i % 4] || orig_data[i] != 1u8 {
                //println!("err {} {}", orig_data[i], i);
                log_exist = true;
                break;
            }
        };
        let ret = if !log_exist {
            for log_id in log_node_at..log_node_at + NODE_FOR_RECOVERY {
                read_block_raw(log_id, &mut orig_data).unwrap();
                let cached_node: u32;
                unsafe {
                    cached_node = *((&orig_data[NODE_SIZE - 4])  as *const _ as *mut u32);
                    *((&orig_data[NODE_SIZE - 4])  as *const _ as *mut u32) = 0u32;
                }
                write_block_raw(cached_node as usize, &mut orig_data).unwrap();
            }
            if log_node_at + self.superblock.log_size as usize >= NODE_NUM_TOTAL {
                self.dnode_bitmap.alloc_log(self.superblock.dnode_max as usize, NODE_NUM_TOTAL - self.superblock.dnode_max as usize + 1);
            }
            println!("[rvd] recovered");
            -1
        } else {
            println!("[rvd] log is clean.");
            0
        };
        println!("[rvd] recover end");
        ret
    }
}

const MAGIC_BARRIER:[u8;4] = [27u8, 119u8, 7u8, 69u8];

pub fn check_file_system_existed() -> i32 {
    if let Ok(_f) = File::open(FILE_SYSTEM_PATH) {
        0
    } else {
        -1
    }
}

pub fn alloc_inode() -> isize {
    DEVICE_MANAGER.lock().unwrap().alloc_inode()
}

pub fn unalloc_inode(node_id: u32) -> isize {
    DEVICE_MANAGER.lock().unwrap().unalloc_inode(node_id)
}

pub fn alloc_dnode() -> isize {
    DEVICE_MANAGER.lock().unwrap().alloc_dnode()
}

pub fn unalloc_dnode(node_id: u32) -> isize {
    DEVICE_MANAGER.lock().unwrap().unalloc_dnode(node_id)
}

pub fn read_block_safe(block_id:usize, buf: &mut [u8]) {
    DEVICE_MANAGER.lock().unwrap().read_block_safe(block_id, buf)
}

pub fn write_block_safe(block_id:usize, buf: &mut [u8]) {
    DEVICE_MANAGER.lock().unwrap().write_block_safe(block_id, buf)
}

pub fn fsync_rust() {
    DEVICE_MANAGER.lock().unwrap().fsync()
}

pub fn get_root_dir_inode_id() -> isize {
    DEVICE_MANAGER.lock().unwrap().get_root_dir_inode_id()
}
