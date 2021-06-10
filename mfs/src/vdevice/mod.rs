use std::fs::{File, OpenOptions};
use std::sync::Mutex;
use std::io::{self, Error, ErrorKind, Write, Read, Seek, SeekFrom};

use super::structure::{NODE_SIZE, NODE_NUM_TOTAL};
use lazy_static::*;

lazy_static! {
    pub static ref DEVICE_FILE:Mutex<File> = Mutex::new({
        let f = if let Ok(try_f) = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .open("./file_system") {
                try_f
            } else {
                File::create("./file_system").unwrap()
            };
        f.set_len((NODE_SIZE * NODE_NUM_TOTAL) as u64).unwrap();
        f
    });
}

/*
fn print_type_of<T>(_: &T) {
    println!("{}", std::any::type_name::<T>())
}
*/
pub fn read_block_at(block_id:usize, buf: &mut [u8]) -> io::Result<()> {
    //print_type_of(&DEVICE_FILE);
    let mut file = DEVICE_FILE.lock().unwrap();
    file.seek(SeekFrom::Start((block_id * NODE_SIZE) as u64)).unwrap();
    let read_size = file.read(buf).unwrap();
    if read_size == NODE_SIZE {
        Ok(())
    } else {
        Err(Error::from(ErrorKind::Other))
    }  
}

pub fn write_block_at(block_id:usize, buf: &mut [u8]) -> io::Result<()> {
    //print_type_of(&DEVICE_FILE);
    let mut file = DEVICE_FILE.lock().unwrap();
    file.seek(SeekFrom::Start((block_id * NODE_SIZE) as u64)).unwrap();
    unsafe {println!("write buf addr {}", buf.as_ptr() as usize);}
    let write_size = file.write(buf).unwrap();
    if write_size == NODE_SIZE {
        Ok(())
    } else {
        Err(Error::from(ErrorKind::Other))
    }
}