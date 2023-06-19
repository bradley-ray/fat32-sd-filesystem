#include "editor.h"
#include "fat32.h"
#include "hal.h"
#include <stdio.h>

// TODO: probably should flush buffer before writing
void minied_main(uint8_t* tx_buff, uint8_t* rx_buff) {
	printf("Welcome to mini editor tool!\n");
	uint8_t filename[16] = {0};
	uint8_t inp = 0;
	minied_buff_t buff;
	for(uint32_t i = 0; i < 512; ++i)
		((uint8_t*)&buff)[i] = 0;
	buff.line1[0] = '\n';
	buff.line2[0] = '\n';
	buff.line3[0] = '\n';
	buff.line4[0] = '\n';
	uint8_t* line = buff.line1;
	while(inp != 'q') {
		inp = uart2_read_byte();
		(void)uart2_read_byte();
		// TODO: all of these commands should be broken up and handled in seperate functions
		switch(inp) {
			case '1':
				printf("1\n");
				line = buff.line1;
				break;
			case '2':
				printf("2\n");
				line = buff.line2;
				break;
			case '3':
				printf("3\n");
				line = buff.line3;
				break;
			case '4':
				printf("4\n");
				line = buff.line4;
				break;
			case '.':
				printf(".\n");
				uart2_read_buff_until('\n', rx_buff, 512);
				for(uint32_t i = 0; i < 127 && rx_buff[i] != '\n'; ++i) // only get 127 characters, 128 will be forced to \n
					line[i] = rx_buff[i];
				line[127] = '\n';
				break;
			case 'n':
				printf("next buff, not implemented yet :(\n");
				break;
			case 'p':
				printf("prev buff, not implemented yet :(\n");
				break;
			case 'w':
				uart2_read_buff_until('\n', rx_buff, 512);
				uint32_t fn_size = 0; 
				for(; fn_size < 16 && rx_buff[fn_size] != '\n'; ++fn_size)
					filename[fn_size] = rx_buff[fn_size];

				for(uint32_t i = 0; i < 512; ++i)
					tx_buff[i] = 0;

				// TODO: stop writing null bytes
				uint32_t f_size = 0;
				for(uint32_t i =0; i < 128 && buff.line1[i] != '\n' && buff.line1[i] != 0; ++f_size, ++i)
					tx_buff[f_size] = buff.line1[i];
				tx_buff[f_size++] = '\n';
				for(uint32_t i =0; i < 128 && buff.line2[i] != '\n' && buff.line2[i] != 0; ++f_size, ++i)
					tx_buff[f_size] = buff.line2[i];
				tx_buff[f_size++] = '\n';
				for(uint32_t i =0; i < 128 && buff.line3[i] != '\n' && buff.line3[i] != 0; ++f_size, ++i)
					tx_buff[f_size] = buff.line3[i];
				tx_buff[f_size++] = '\n';
				for(uint32_t i =0; i < 128 && buff.line4[i] != '\n' && buff.line4[i] != 0; ++f_size, ++i)
					tx_buff[f_size] = buff.line4[i];
				tx_buff[f_size++] = '\n';

				uint32_t cluster = fat_find_free_cluster();
				fat_create_file(filename, fn_size, ATTR_ARCHIVE, cluster, f_size, rx_buff);
				fat_write_file(tx_buff, cluster);
				break;
			case '\'':
				for(uint32_t i = 0; i < 128; ++i)
					printf("%c", line[i]);
				break;
			case ',':
				for(uint32_t i = 0; i < 128; ++i)
					printf("%c", buff.line1[i]);
				for(uint32_t i = 0; i < 128; ++i)
					printf("%c", buff.line2[i]);
				for(uint32_t i = 0; i < 128; ++i)
					printf("%c", buff.line3[i]);
				for(uint32_t i = 0; i < 128; ++i)
					printf("%c", buff.line4[i]);
				break;
			case 'q':
				printf("leaving minied...\n");
				break;
			default:
				printf("unknown minied cmd...\n");
				break;
		}
	}
}