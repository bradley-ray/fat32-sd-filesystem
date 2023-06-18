#ifndef __HELPERS_h__
#define __HELPERS_h__

#include <inttypes.h>

uint8_t str_eq(uint8_t* str1, uint32_t str1_size, uint8_t* str2, uint32_t str2_size);
uint32_t from_little_endian_32(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);
uint32_t from_little_endian_16(uint8_t byte0, uint8_t byte1);

#endif