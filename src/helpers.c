#include "helpers.h"
#include <stdio.h>

uint8_t str_eq(uint8_t* str1, uint32_t str1_size, uint8_t* str2, uint32_t str2_size) {
	if (str1_size != str2_size) 
		return 0;

	for (uint32_t i = 0; i < str1_size; ++i) {
		if (str1[i] != str2[i]) {
			return 0;
		}
	}

	return 1;
}

uint32_t from_little_endian_32(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
	return ((uint32_t)byte3 << 24)+ ((uint32_t)byte2 << 16) + ((uint32_t)byte1 << 8) + (uint32_t)byte0;
}

uint32_t from_little_endian_16(uint8_t byte0, uint8_t byte1) {
	return ((uint32_t)byte1 << 8) + (uint32_t)byte0;
}