#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdint.h>

void list_dir(uint8_t* buff);
void change_dir(char* dirname, uint8_t* buff);
void cat_file(char* filename, uint8_t* buff);
void print_current_dir(void);
void edit_file(uint8_t* tx_buff, uint8_t* rx_buff);
void create_file(char* filename, uint8_t* buff);
void delete_file(char* filename, uint8_t* buff);
void make_dir(char* dirname, uint8_t* buff);
void delete_dir(char* dirname, uint8_t* buff);

#endif