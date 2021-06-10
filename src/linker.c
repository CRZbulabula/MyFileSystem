#include "linker.h"

void test_rust() {
    test_(2,3);
    return ;
}

void read_block_at(uint32_t block_id, const uint8_t* buf_ptr)
{
    read_block(block_id, buf_ptr);
}

void write_block_at(uint32_t block_id, const uint8_t* buf_ptr)
{
    write_block(block_id, buf_ptr);
}