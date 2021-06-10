#ifndef _LINKER_H_
#define _LINKER_H_

#include <stdint.h>

extern void test_(uint32_t a, uint32_t b);
extern void write_block(uint32_t block_id, const uint8_t* buf_ptr);
extern void read_block(uint32_t block_id, const uint8_t* buf_ptr);

void test_rust();

void read_block_at(uint32_t block_id, const uint8_t* buf_ptr);
void write_block_at(uint32_t block_id, const uint8_t* buf_ptr);

#endif
