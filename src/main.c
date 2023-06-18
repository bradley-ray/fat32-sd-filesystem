#include <stdio.h>
#include "hal.h"
#include "sd.h"
#include "fat32.h"
#include "helpers.h"
#include "init.h"


void device_init(void);
void run_shell(void);

// General buffers
static uint8_t tx_buff[512] = {0};
static uint8_t rx_buff[512] = {0};

int main() {
	device_init();
	run_shell();
}

void device_init(void) {
	uart2_init(115200);
	(void)uart2_read_byte();

	delay(10);

	sd_preinit();
	SPI1->CR2 |= BIT(2);
	sd_init(rx_buff);
	fat_init(rx_buff);
	spi_init();
}

// TOOD: this needs to be cleaned up a lot
void run_shell(void) {
	while(1) {	
		uart2_read_buff_until('\n', rx_buff, 512);

		uint8_t cmd_size = 0;
		uint8_t arg_size = 0;
		for(uint32_t i = 0; i < 512 && rx_buff[i] != '\n' && rx_buff[i] != ' '; ++i, ++cmd_size);

		if (str_eq((uint8_t*)"ls", 2, rx_buff, cmd_size)) {
			list_dir(rx_buff);
		} else if (str_eq((uint8_t*)"cd", 2, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				change_dir("root", rx_buff);
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			change_dir((char*)(rx_buff+cmd_size+1), rx_buff);
		} else if (str_eq((uint8_t*)"mkdir", 5, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				printf("invalid argument\n");
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			make_dir((char*)(rx_buff+cmd_size+1), rx_buff);
		} else if (str_eq((uint8_t*)"cat", 3, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				printf("invalid argument\n");
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			cat_file((char*)(rx_buff+cmd_size+1), rx_buff);
		} else if (str_eq((uint8_t*)"ed", 2, rx_buff, cmd_size)) {
			edit_file(tx_buff, rx_buff);
		} else if (str_eq((uint8_t*)"pwd", 3, rx_buff, cmd_size)) {
			print_current_dir();
		} else if (str_eq((uint8_t*)"touch", 5, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				printf("invalid argument\n");
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			create_file((char*)(rx_buff+cmd_size+1), rx_buff);
		} else if (str_eq((uint8_t*)"rm", 2, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				printf("invalid argument\n");
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			delete_file((char*)(rx_buff+cmd_size+1), rx_buff);
		} else if (str_eq((uint8_t*)"rmdir", 5, rx_buff, cmd_size)) {
			if (rx_buff[cmd_size] == '\n') {
				printf("invalid argument\n");
				continue;
			}
			for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
			rx_buff[cmd_size+1+arg_size] = 0;
			delete_dir((char*)(rx_buff+cmd_size+1), rx_buff);
		} else {
			printf("unknown command...\n");
		}

		printf("-----------------\n");
		fat_cache_fat(rx_buff);
	}
}