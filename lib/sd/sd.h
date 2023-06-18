#ifndef __SD_h__
#define __SD_h__

#include <stdint.h>

#define SD_CMD0_CRC 0x95
#define SD_CMD8_CRC 0x87

typedef enum {
    SD_CMD0,
    SD_CMD8 = 8,
    SD_CMD9,
    SD_CMD17 = 17,
    SD_CMD24 = 24,
    SD_CMD55 = 55,
    SD_CMD58 = 58,
    SD_ACMD41 = 41,
} sd_cmd_t;

void sd_write(uint8_t* tx_buff, uint32_t size);
void sd_read(uint8_t* rx_buff, uint32_t size);

void sd_preinit(void);
void sd_init(uint8_t* rx_buff);
uint8_t sd_send_cmd(uint8_t* cmd);
void sd_read_block(uint32_t addr, uint8_t* buff);
void sd_write_block(uint32_t addr, uint8_t* buff);



#endif