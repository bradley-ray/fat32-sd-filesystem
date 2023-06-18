#ifndef __SD_h__
#define __SD_h__

#include <inttypes.h>
#include <stdio.h>

#define SD_CMD0_CRC 0x95
#define SD_CMD8_CRC 0x87

typedef struct {
    uint8_t cmd;
    uint8_t arg1;
    uint8_t arg2;
    uint8_t arg3;
    uint8_t arg4;
    uint8_t crc;
    uint8_t resp_type;
} sd_cmd_struct_t;

typedef enum {
    SD_CMD0,
    SD_CMD1,
    SD_CMD2,
    SD_CMD3,
    SD_CMD4,
    SD_CMD5,
    SD_CMD6,
    SD_CMD7,
    SD_CMD8,
    SD_CMD9,
    SD_CMD10,
    SD_CMD11,
    SD_CMD12,
    SD_CMD13,

    SD_CMD15 = 15,
    SD_CMD16,
    SD_CMD17,
    SD_CMD18,

    SD_CMD20 = 20,

    SD_CMD24 = 24,
    SD_CMD25,
    SD_CMD26,
    SD_CMD27,
    SD_CMD28,
    SD_CMD29,
    SD_CMD30,
    
    SD_CMD32 = 32,
    SD_CMD33,
    
    SD_CMD38 = 38,
    SD_CMD39,
    SD_CMD40,

    
    SD_CMD42 = 42,

    SD_CMD55 = 55,
    SD_CMD56,
    
    SD_CMD58 = 58,
    SD_CMD59,

    SD_ACMD41 = 41,
} sd_cmd_t;

typedef struct {
    uint8_t version: 2;
    uint8_t reserved_1: 6;
    uint8_t taac;
    uint8_t nsac;
    uint8_t tran_speed;
    uint32_t ccc: 12;
    uint8_t read_bl_len: 4;
    uint8_t read_bl_partial: 1;
    uint8_t write_blk_misalign: 1;
    uint8_t read_blk_misalign: 1;
    uint8_t dsr_imp: 1;
    uint8_t reserved_2: 6;
    uint32_t c_size: 22;
    uint32_t one;
    uint32_t two;
    uint32_t three;
    uint32_t four;
    uint32_t five;
    uint32_t six;
} csd_response_t;


uint8_t sd_send_cmd(uint8_t* cmd);
void sd_preinit(void);
void sd_init(void);
void sd_read_block(uint32_t addr, uint8_t* buff);
void sd_write_block(uint32_t addr, uint8_t* buff);


#endif