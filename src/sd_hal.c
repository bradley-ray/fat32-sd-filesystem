#include "hal.h"

void sd_write(uint8_t* tx_buff, uint32_t size) {
    spi_read_write_buff(tx_buff, (void*)0, size);
}

// TODO: need to work on spi in hal to make things like this less awkward
void sd_read(uint8_t* rx_buff, uint32_t size){
    for (uint32_t i = 0; i < size; i++)
        rx_buff[i] = spi_read_write_byte(0xff);
}