#include "sd.h"
#include "hal.h"

static uint8_t sd_cmd0[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
static uint8_t sd_cmd8[6] = {0x48, 0x00, 0x00, 0x01, 0xaa, 0x87};
static uint8_t sd_cmd58[6] = {0x7a, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd55[6] = {0x77, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_acmd41[6] = {0x69, 0x40, 0x00, 0x00, 0x00, 0xff};
// static uint8_t sd_cmd9[6] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd17[6] = {0x40 | 17, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd24[6] = {0x40 | 24, 0x00, 0x00, 0x00, 0x00, 0xff};
// static uint8_t sd_cmd13[6] = {0x40 | 13, 0x00, 0x00, 0x00, 0x00, 0xff};
// static uint8_t sd_acmd22[6] = {0x40 | 22, 0x00, 0x00, 0x00, 0x00, 0xff};

uint8_t sd_send_cmd(uint8_t* cmd) {
	for(uint32_t i = 0; spi_read_write_byte(0xff) != 0xff && i < 512; ++i);

	spi_read_write_buff(cmd, NULL, 6);

	uint8_t response;
	while((response = spi_read_write_byte(0xff)) & 0x80);

	return response;
}

void sd_preinit(void) {
	for (uint32_t i = 0; i < 10; ++i)
		spi_read_write_byte(0xff);
}

void sd_init(void) {
	// send CMD0
	while(sd_send_cmd(sd_cmd0) != 0x1);

	// send CMD8
	while(sd_send_cmd(sd_cmd8) != 0x1);
	for (uint32_t i = 0; i < 4; ++i)
		spi_read_write_byte(0xff);

	// send CMD58 get device data
	while(sd_send_cmd(sd_cmd58) != 0x1);
	for (uint32_t i = 0; i < 4; ++i)
		spi_read_write_byte(0xff);

	// send CMD55, ACMD41 until not idle
	do {
		sd_send_cmd(sd_cmd55);
	} while (sd_send_cmd(sd_acmd41) != 0);

	// send CMD58
	while(sd_send_cmd(sd_cmd58) != 0x0);
	for (uint32_t i = 0; i < 4; ++i)
		spi_read_write_byte(0xff);
}

void sd_read_block(uint32_t addr, uint8_t* buff) {
	sd_cmd17[1] = (uint8_t)((addr) >> 24);
	sd_cmd17[2] = (uint8_t)((addr) >> 16);
	sd_cmd17[3] = (uint8_t)((addr) >> 8);
	sd_cmd17[4] = (uint8_t)((addr));

	while(sd_send_cmd(sd_cmd17) != 0);
	while(spi_read_write_byte(0xff) != 0xfe);

	for (uint32_t i = 0; i < 512; ++i)
		buff[i] = 0;
	for (uint32_t i = 0; i < 512; ++i) 
		buff[i] = spi_read_write_byte(0xff);
}

void sd_write_block(uint32_t addr, uint8_t* buff) {
	sd_cmd24[1] = (uint8_t)((addr) >> 24);
	sd_cmd24[2] = (uint8_t)((addr) >> 16);
	sd_cmd24[3] = (uint8_t)((addr) >> 8);
	sd_cmd24[4] = (uint8_t)((addr));

	while(sd_send_cmd(sd_cmd24) != 0);
	spi_read_write_byte(0xfe);
	spi_read_write_buff(buff, NULL, 512);
	while(spi_read_write_byte(0xff) & 0x10);
}