#include "helpers.h"

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