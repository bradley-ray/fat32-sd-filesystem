#include "sd.h"
#include <stdint.h>

void fat_write_block(uint32_t addr, uint8_t* buff) {
    sd_write_block(addr, buff);
}

void fat_read_block(uint32_t addr, uint8_t* buff) {
    sd_read_block(addr, buff);
}