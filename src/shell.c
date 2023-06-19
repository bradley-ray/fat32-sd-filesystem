#include "shell.h"
#include "fat32.h"
#include "editor.h"
#include "helpers.h"
#include "sd.h"
#include <stdio.h>

void list_dir(uint8_t* buff) {
	uint32_t sector = calc_first_sector(fat_get_current_dir_cluster());
	sd_read_block(sector, buff);

    uint32_t cluster = fat_get_current_dir_cluster();
    directory_t dir;
    for(uint32_t i = 0; i != 512; i = fat_find_next_dir_entry(&dir, cluster, i, buff))
        printf("<dir> --  %s\n", dir.name);

    file_t file;
    for(uint32_t i = 0; i != 512; i = fat_find_next_file_entry(&file, cluster, i, buff))
        printf("<file> -- %s\n", file.name);
}

void change_dir(char* dirname, uint8_t* buff) {
	uint8_t name1_size = 0;
	uint8_t new_dir[16] = {0};
	for(uint32_t i = 0; i < 16 && dirname[i] != 0; i++, name1_size++)
		new_dir[i] = dirname[i];

	if (str_eq((uint8_t*)dirname, name1_size, (uint8_t*)".", 1)) {
		return;
	}

	if(str_eq((uint8_t*)dirname, name1_size, (uint8_t*)"root", 4)) {
		fat_set_dir_root();	
		return;
	}

	if (str_eq((uint8_t*)new_dir, name1_size, (uint8_t*)"..", 2)) {
		name1_size = 11;
		for(uint32_t i = 2; i < 11; ++i)
			new_dir[i] = ' ';
	}

	uint32_t i = fat_find_entry_idx(new_dir, name1_size, fat_get_current_dir_cluster(), buff);
	directory_t directory;
	read_dir(&directory, buff, i);
    fat_update_current_dir(&directory);
}

void cat_file(char* filename, uint8_t* buff) {
	uint8_t name1_size = 0;
	uint8_t file_name[16] = {0};
	for(uint32_t j = 0; j < 16 && filename[j] != 0; j++, name1_size++)
		file_name[j] = filename[j];

	uint32_t i = fat_find_entry_idx(file_name, name1_size, fat_get_current_dir_cluster(), buff);
	if (i == 512) {
		printf("file '%s' not found\n", file_name);
		return;
	}

	file_t file;
	read_file(&file, buff, i);
	uint32_t sector = calc_first_sector(file.cluster);
	for (uint32_t j = 0, bytes_rem = file.filesize; j < 64 && bytes_rem > 0; ++j) {
		sd_read_block(sector+j, buff);
		for(uint32_t k = 0; k < 512 && bytes_rem > 0; ++k, bytes_rem--)
			printf("%c", buff[k]);

		if (bytes_rem == 0) break;
	}
	printf("\n");
}

void create_file(char* filename, uint8_t* buff) {
	uint32_t size = 0;
	for(uint32_t j = 0; j < 16 && filename[j] != 0; j++, size++);

    uint32_t cluster = fat_find_free_cluster();
	fat_create_file((uint8_t*)filename, size, ATTR_ARCHIVE, cluster, 0, buff);
}

void delete_file(char* filename, uint8_t* buff) {
	uint32_t size = 0;
	for(uint32_t j = 0; j < 16 && filename[j] != 0; j++, size++);

	fat_delete_file((uint8_t*)filename, size, buff);
}

void delete_dir(char* dirname, uint8_t* buff) {
	uint32_t size = 0;
	for(uint32_t j = 0; j < 16 && dirname[j] != 0; j++, size++);

	fat_delete_dir((uint8_t*)dirname, size, buff);
}

void print_current_dir(void) {
	printf("Dir: %s\n", fat_get_current_dir_name());
}

void edit_file(uint8_t* tx_buff, uint8_t* rx_buff) {
	minied_main(tx_buff, rx_buff);
}


void make_dir(char* dirname, uint8_t* buff) {
	uint32_t size = 0;
	for(uint32_t j = 0; j < 16 && dirname[j] != 0; j++, size++);


    uint32_t cluster = fat_find_free_cluster();
	fat_create_file((uint8_t*)dirname, size, ATTR_DIRECTORY, cluster, 0, buff);

	// create the '.' and '..' directories
	fat_create_curr_dir(cluster, buff);
	fat_create_prev_dir(cluster, buff);
	
}