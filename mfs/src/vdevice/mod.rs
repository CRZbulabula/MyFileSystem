const FILE_SYSTEM_PATH: &str = "./file_system";

mod fs_file;
mod manager;

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
