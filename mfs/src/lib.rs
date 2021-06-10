extern crate libc;

mod structure;
mod vdevice;

use std::sync::Mutex;
use std::boxed::Box;
use std::slice;
use structure::{NODE_SIZE};
use vdevice::{read_block_at, write_block_at};

//static mut static_buf:Mutex<Box<[u8]>> = Mutex::new(Box::new([0u8; NODE_SIZE]));

#[no_mangle]
pub extern "C" fn test_(a: u32, b: u32) {
    println!("print : a {}+ b{} = {}", a, b, a + b);
}

#[no_mangle]
pub extern "C" fn read_block(block_id: usize, buf: usize) {
    let orig_buf = unsafe {
        slice::from_raw_parts_mut(buf as *mut u8, NODE_SIZE)
    };
    read_block_at(block_id, orig_buf).unwrap();
}

#[no_mangle]
pub extern "C" fn write_block(block_id: usize, buf: *const u8) {
    let orig_buf = unsafe {
        //println!("before trans");
        //println!("first v {}", *(buf as *const usize));
        //println!("after trans");
        slice::from_raw_parts_mut(buf as *mut u8, NODE_SIZE)
    };
    write_block_at(block_id, orig_buf).unwrap();
}

#[test]
fn test_read_write() {
    let mut buf = [0u8; NODE_SIZE];
    buf[3] = 7;
    buf[6] = 27;
    write_block_at(0, &mut buf).unwrap();
    buf[3] = 4;
    buf[6] = 13;
    write_block_at(4, &mut buf).unwrap();
    read_block_at(0, &mut buf).unwrap();
    println!("{} {}", buf[3], buf[6]);
    assert_eq!(buf[3], 7);
    assert_eq!(buf[6], 27);
    read_block_at(4, &mut buf).unwrap();
    println!("{} {}", buf[3], buf[6]);
    assert_eq!(buf[3], 4);
    assert_eq!(buf[6], 13);
}