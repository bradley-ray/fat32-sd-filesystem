#include "sd.h"
#include "hal.h"

static uint8_t sd_cmd0[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
static uint8_t sd_cmd8[6] = {0x48, 0x00, 0x00, 0x01, 0xaa, 0x87};
static uint8_t sd_cmd58[6] = {0x7a, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd55[6] = {0x77, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_acmd41[6] = {0x69, 0x40, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd17[6] = {0x40 | 17, 0x00, 0x00, 0x00, 0x00, 0xff};
static uint8_t sd_cmd24[6] = {0x40 | 24, 0x00, 0x00, 0x00, 0x00, 0xff};

uint8_t sd_send_cmd(uint8_t* cmd) {
	// wait until not busy
	uint8_t response;
	do {
		sd_read(&response, 1);
	} while(response != 0xff);

	// send cmd
	sd_write(cmd, 6);

	// wait for R1 response
	do {
		sd_read(&response, 1);
	} while(response & 0x80);

	return response;
}

// TODO: should this be part of sd hal layer?
// make sure CS is high when called
void sd_preinit(void) {
	uint8_t cmd = 0xff;
	for (uint32_t i = 0; i < 10; ++i)
		sd_write(&cmd, 1);
}

// SD Card initialization sequence
void sd_init(uint8_t* rx_buff) {
	while(sd_send_cmd(sd_cmd0) != 0x1);

	while(sd_send_cmd(sd_cmd8) != 0x1);
	sd_read(rx_buff, 4);

	while(sd_send_cmd(sd_cmd58) != 0x1);
	sd_read(rx_buff, 4);

	do {
		sd_send_cmd(sd_cmd55);
	} while (sd_send_cmd(sd_acmd41) != 0);

	while(sd_send_cmd(sd_cmd58) != 0x0);
	sd_read(rx_buff, 4);
}

// Read a block from SD Card Memory
void sd_read_block(uint32_t addr, uint8_t* rx_buff) {
	sd_cmd17[1] = (uint8_t)((addr) >> 24);
	sd_cmd17[2] = (uint8_t)((addr) >> 16);
	sd_cmd17[3] = (uint8_t)((addr) >> 8);
	sd_cmd17[4] = (uint8_t)((addr));

	while(sd_send_cmd(sd_cmd17) != 0);
	uint8_t response;
	do {
		sd_read(&response, 1);
	} while(response != 0xfe);

	sd_read(rx_buff, 512);
}

// Write a block to SD Card Memory
void sd_write_block(uint32_t addr, uint8_t* tx_buff) {
	sd_cmd24[1] = (uint8_t)((addr) >> 24);
	sd_cmd24[2] = (uint8_t)((addr) >> 16);
	sd_cmd24[3] = (uint8_t)((addr) >> 8);
	sd_cmd24[4] = (uint8_t)((addr));

	while(sd_send_cmd(sd_cmd24) != 0);

	uint8_t byte = 0xfe;
	sd_write(&byte, 1);
	sd_write(tx_buff, 512);

	uint8_t response;
	do {
		sd_read(&response, 1);
	} while(response & 0x10);
}