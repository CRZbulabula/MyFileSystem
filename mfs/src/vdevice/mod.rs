const FILE_SYSTEM_PATH: &str = "./file_system";

mod disk_crypto;
mod fs_file;
mod manager;

use disk_crypto::{BLOCK_CRYPTOR};
use fs_file::*;
//use manager::*;

pub use manager::{
    check_file_system_existed,
    alloc_inode,
    unalloc_inode,
    alloc_dnode,
    unalloc_dnode,
    read_block_safe,
    write_block_safe,
    fsync_rust,
    get_root_dir_inode_id,
};

#[test]
fn test_log_unsafe_filling() {
    let x = [7u8;6];
    let y = 12345u32;
    let z = &y;
    unsafe {
    *((&x[2])  as *const _ as *mut u32) = *z;
    }
    assert_eq!(x[2], (z % (1<<8)) as u8);
    assert_eq!(x[3], (z / (1<<8) % (1<<8)) as u8);
    //println!("{}, {},{},{},{},{},{}", x[0],x[1],x[2],x[3],x[4],x[5], y);
}