use crypto::aessafe::{AesSafe256Encryptor, AesSafe256Decryptor};
use crypto::symmetriccipher::{BlockDecryptor, BlockEncryptor};
use std::sync::Mutex;
use std::slice;
use lazy_static::*;

use crate::structure::NODE_SIZE;

const SYSTEM_KEY:[u64;4] = [4179790572462815680u64,7581474513761441404u64, 2411597601701368129u64, 2168059762611313795u64];
const BLOCK_SIZE:usize = 16;
const BLOCK_NUM:usize = NODE_SIZE / BLOCK_SIZE;

lazy_static! {
    
    pub static ref BLOCK_CRYPTOR:Mutex<BlockCryptor> = Mutex::new(BlockCryptor::new(
        unsafe {&slice::from_raw_parts(SYSTEM_KEY.as_ptr() as *const u8, 32)}
    ));   
}


pub struct BlockCryptor {
    encryptor: AesSafe256Encryptor,
    decryptor: AesSafe256Decryptor,
    pub buf: [u8;NODE_SIZE],
}

impl BlockCryptor {
    pub fn new(key: &[u8]) -> Self {
        Self {
            encryptor: AesSafe256Encryptor::new(key),
            decryptor: AesSafe256Decryptor::new(key),
            buf: [0u8;NODE_SIZE],
        }
    }

    pub fn decrypt(&mut self, output: &mut [u8]) {
        for i in 0..BLOCK_NUM {
            self.decryptor.decrypt_block(&self.buf[i*BLOCK_SIZE..(i+1)*BLOCK_SIZE], &mut output[i*BLOCK_SIZE..(i+1)*BLOCK_SIZE]);
        }
    }

    pub fn encrypt(&mut self, input: &[u8]) {
        for i in 0..BLOCK_NUM {
           self.encryptor.encrypt_block(&input[i*BLOCK_SIZE..(i+1)*BLOCK_SIZE], &mut self.buf[i*BLOCK_SIZE..(i+1)*BLOCK_SIZE]);
        }
    }


}

impl Drop for BlockCryptor {
    fn drop(&mut self) {
        self.buf = [0u8;NODE_SIZE]; //清空，防止留在内存中被窃取
    }
}
