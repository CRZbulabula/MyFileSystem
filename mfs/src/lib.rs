extern crate libc;

mod structure;
mod vdevice;

//use std::sync::Mutex;
//use std::boxed::Box;
use std::slice;
use structure::{NODE_SIZE};
/*
use vdevice::{
    check_file_system_existed,
    alloc_inode,
    unalloc_inode,
    alloc_dnode,
    unalloc_dnode,
    read_block_safe,
    write_block_safe,
    fsync_rust,
};*/

//static mut static_buf:Mutex<Box<[u8]>> = Mutex::new(Box::new([0u8; NODE_SIZE]));

#[no_mangle]
pub extern "C" fn test_(a: u32, b: u32) {
    println!("print : a {}+ b{} = {}", a, b, a + b);
}

#[no_mangle]
pub extern "C" fn check_file_system_existed() -> i32 {
    vdevice::check_file_system_existed()
}

#[no_mangle]
pub extern "C" fn alloc_inode() -> i32 {
    vdevice::alloc_inode() as i32
}

#[no_mangle]
pub extern "C" fn unalloc_inode(node_id: u32) -> i32 {
    vdevice::unalloc_inode(node_id) as i32
}

#[no_mangle]
pub extern "C" fn alloc_dnode() -> i32 {
    vdevice::alloc_dnode() as i32
}

#[no_mangle]
pub extern "C" fn unalloc_dnode(node_id: u32) -> i32 {
    vdevice::unalloc_dnode(node_id) as i32
}

#[no_mangle]
pub extern "C" fn read_block(block_id: usize, buf: *const u8) {
    let orig_buf = unsafe {
        slice::from_raw_parts_mut(buf as *mut u8, NODE_SIZE)
    };
    vdevice::read_block_safe(block_id, orig_buf);
}

#[no_mangle]
pub extern "C" fn write_block(block_id: usize, buf: *const u8) {
    let orig_buf = unsafe {
        //println!("before trans");
        //println!("first v {}", *(buf as *const usize));
        //println!("after trans");
        slice::from_raw_parts_mut(buf as *mut u8, NODE_SIZE)
    };
    vdevice::write_block_safe(block_id, orig_buf);
}

#[no_mangle]
pub extern "C" fn fsync_rust()  {
    vdevice::fsync_rust()
}

#[no_mangle]
pub extern "C" fn get_root_dir_inode_id() -> i32 {
    vdevice::get_root_dir_inode_id() as i32
}

#[test]
fn test_read_write() {
    let mut buf = [0u8; NODE_SIZE];
    buf[3] = 7;
    buf[6] = 27;
    vdevice::write_block_safe(0, &mut buf);
    buf[3] = 4;
    buf[6] = 13;
    vdevice::write_block_safe(4, &mut buf);
    vdevice::read_block_safe(0, &mut buf);
    println!("{} {}", buf[3], buf[6]);
    assert_eq!(buf[3], 7);
    assert_eq!(buf[6], 27);
    vdevice::read_block_safe(4, &mut buf);
    println!("{} {}", buf[3], buf[6]);
    assert_eq!(buf[3], 4);
    assert_eq!(buf[6], 13);
}