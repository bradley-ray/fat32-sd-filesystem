#include "fat32.h"
#include "sd.h"
#include "hal.h"
#include "helpers.h"
#include <stdio.h>

static uint32_t bpb_SecPerClus;
static uint32_t FirstDataSector;

static ldir_entry_t long_dir;
static sdir_entry_t short_dir;
static directory_t current_dir;
static uint32_t fat_cache[FAT_CACHE_CAPACITY];

static uint8_t is_empty(uint8_t byte) {
	return (byte == 0) || (byte == 0xe5);
}

static uint8_t is_long_name(uint8_t attr) {
    return (attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME;
}

static uint8_t is_dir(uint8_t attr1, uint8_t attr2) {
	return (attr1 & ATTR_DIRECTORY) || (is_long_name(attr1) && (attr2 & ATTR_DIRECTORY));
}

static uint8_t is_file(uint8_t attr1, uint8_t attr2) {
	return (attr1 & ATTR_ARCHIVE) || (is_long_name(attr1) && (attr2 & ATTR_ARCHIVE));
}


void read_dir(directory_t* dir_handle, ldir_entry_t* ldir_handle, sdir_entry_t* sdir_handle, uint8_t* buff, uint32_t idx) {
    for(uint32_t j = 0; j < 32; ++j) {
        ((uint8_t*)ldir_handle)[j] = buff[j+idx];
	}

	for(uint32_t i = 0; i < 16; ++i) {
		dir_handle->name[i] = 0;
	}

    if (is_long_name(ldir_handle->LDIR_Attr)) {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)sdir_handle)[j] = buff[j+idx+32];
        for(uint32_t j = 0, k = 0; j < 10; j+=2, k++)
            dir_handle->name[k] = ldir_handle->LDIR_Name1[j];
        for(uint32_t j = 0, k = 5; j < 12; j+=2, k++)
            dir_handle->name[k] = ldir_handle->LDIR_Name2[j];
        for(uint32_t j = 0, k = 11; j < 4; j+=2, k++)
            dir_handle->name[k] = ldir_handle->LDIR_Name3[j];
    } else {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)sdir_handle)[j] = buff[j+idx];
        for(uint32_t j = 0; j < 11; ++j)
            dir_handle->name[j] = sdir_handle->DIR_Name[j];
    }
    
	dir_handle->cluster = from_little_endian_32(sdir_handle->DIR_FstClusLO[0], sdir_handle->DIR_FstClusLO[1], sdir_handle->DIR_FstClusHI[0], sdir_handle->DIR_FstClusHI[1]);
	// root cluster is cluster 2, but marked 0 in .. entries
    dir_handle->cluster = (dir_handle->cluster != 0) ? dir_handle->cluster : 2;
}

void read_file(file_t* file_handle, ldir_entry_t* ldir_handle, sdir_entry_t* sdir_handle, uint8_t* buff, uint32_t idx) {
    for(uint32_t j = 0; j < 32; ++j) {
        ((uint8_t*)ldir_handle)[j] = buff[j+idx];
	}

	for(uint32_t i = 0; i < 16; ++i) {
		file_handle->name[i] = 0;
	}

    if (is_long_name(ldir_handle->LDIR_Attr)) {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)sdir_handle)[j] = buff[j+idx+32];
        for(uint32_t j = 0, k = 0; j < 10; j+=2, k++)
            file_handle->name[k] = ldir_handle->LDIR_Name1[j];
        for(uint32_t j = 0, k = 5; j < 12; j+=2, k++)
            file_handle->name[k] = ldir_handle->LDIR_Name2[j];
        for(uint32_t j = 0, k = 11; j < 4; j+=2, k++)
            file_handle->name[k] = ldir_handle->LDIR_Name3[j];
    } else {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)sdir_handle)[j] = buff[j+idx];
        for(uint32_t j = 0; j < 11; ++j)
            file_handle->name[j] = sdir_handle->DIR_Name[j];
    }
    
	file_handle->cluster = from_little_endian_32(sdir_handle->DIR_FstClusLO[0], sdir_handle->DIR_FstClusLO[1], sdir_handle->DIR_FstClusHI[0], sdir_handle->DIR_FstClusHI[1]);
	file_handle->filesize = from_little_endian_32(sdir_handle->DIR_FileSize[0], sdir_handle->DIR_FileSize[1], sdir_handle->DIR_FileSize[2], sdir_handle->DIR_FileSize[3]);
}

uint32_t fat_find_entry_idx(uint8_t* entry_name, uint32_t size, uint32_t cluster, uint8_t* buff) {
	sd_read_block(calc_first_sector(cluster), buff);

	for (uint32_t i = 0; i < 512; i += (!is_long_name(buff[i+11]) ? 32 : 64)) {
		if(is_empty(buff[i])) {
			continue;
		}

		if (is_dir(buff[i+11], buff[i+11+32])) {
			directory_t dir;
			read_dir(&dir, &long_dir, &short_dir, buff, i);
			uint8_t name2_size = 0;
			for(uint32_t j = 0; j < 16 && dir.name[j] != 0; j++, name2_size++);
			if (str_eq(entry_name, size, dir.name, name2_size))
				return i;
		} else if (is_file(buff[i+11], buff[i+11+32])) {
			file_t file;
			read_file(&file, &long_dir, &short_dir, buff, i);
			uint8_t name2_size = 0;
			for(uint32_t j = 0; j < 16 && file.name[j] != 0; j++, name2_size++);
			if (str_eq(entry_name, size, file.name, name2_size))
				return i;
		}
	}

	// NOT FOUND
	return 512;
}

void fat_init(uint8_t* buff) {
	fat_read_boot(buff);
	fat_get_clus_info(buff);
	fat_cache_fat(buff);
	fat_set_dir_root(&current_dir);
	fat_open_dir(&current_dir);
}

void fat_read_boot(uint8_t* buff) {
	sd_read_block(0, buff);
}

void fat_get_clus_info(uint8_t* buff) {
	uint32_t bpb_RootEntCnt = from_little_endian_16(buff[17], buff[18]);
	uint32_t bpb_BytsPerSec = from_little_endian_16(buff[11], buff[12]);
	uint32_t RootDirSectors = ((bpb_RootEntCnt * 32) + (bpb_BytsPerSec - 1)) / bpb_BytsPerSec;
	uint32_t bpb_FatSz16 = from_little_endian_16(buff[22], buff[23]);
	uint32_t bpb_FatSz32 = from_little_endian_32(buff[36], buff[37], buff[38], buff[39]);
	uint32_t FatSz = bpb_FatSz16 ?  bpb_FatSz16 : bpb_FatSz32;
	uint32_t bpb_RsvdSecCnt = from_little_endian_16(buff[14], buff[15]);
	uint32_t bpb_NumFats = buff[16];
	bpb_SecPerClus = buff[13];
	FirstDataSector = bpb_RsvdSecCnt + (bpb_NumFats * FatSz) + RootDirSectors;
}

void fat_set_dir_root(directory_t* directory) {
	directory->name[0] = 'r';
	directory->name[1] = 'o';
	directory->name[2] = 'o';
	directory->name[3] = 't';
	for(uint32_t i = 4; i < 11; i++)
		directory->name[i] = 0;

	directory->cluster = ROOT_CLUSTER;
}

void fat_open_dir(directory_t* directory) {
	(void)directory;
}

void fat_cache_fat(uint8_t* buff) {
	// get FAT
	sd_read_block(64, buff);

	for (uint32_t i = 0; i < 512/4; ++i)
		fat_cache[i] = ((uint32_t*)buff)[i];
}

// TODO: need to clean this up a lot 
// 	* move out creation of shortname and longname to sepearte functions
// 	* write FAT
//  * needs to find next avaiable sector/cluster before assigning
void fat_create_file(uint8_t* filename, uint32_t size, uint8_t type, uint32_t cluster, uint32_t f_size, uint8_t* buff) {
	uint8_t primary[8] = {0};
	uint8_t ext[3] = {0};
	if(size < 11) {
		for(uint32_t i = 0;  i < size && filename[i] != '.'; i++)
			primary[i] = filename[i];
		for(uint32_t i = 7; i > 0 && primary[i] == 0; i--)
			primary[i] = ' ';
	} else {
		for(uint32_t i = 0; i < 6; ++i) {
			primary[i] = filename[i];
		}
		primary[6] = '~';
		primary[7] = '1'; // TODO: need to update this based on other files with simlar shortnames
	}
	ext[0] = (type == ATTR_ARCHIVE) ? filename[size-3] : 0;
	ext[1] = (type == ATTR_ARCHIVE) ? filename[size-2] : 0;
	ext[2] = (type == ATTR_ARCHIVE) ? filename[size-1] : 0;
	for(uint32_t i = 0; i < 8; ++i)
		short_dir.DIR_Name[i] = primary[i];
	for(uint32_t i = 8; i < 11; ++i)
		short_dir.DIR_Name[i] = ext[i-8];
	short_dir.DIR_Attr = type;
	short_dir.DIR_NTres = 0;
	short_dir.DIR_CrtTimeTenth = 0;
	short_dir.DIR_CrtTime[0] = 0;
	short_dir.DIR_CrtTime[1] = 0;
	short_dir.DIR_CrtDate[0] = 0;
	short_dir.DIR_CrtDate[1] = 0;
	short_dir.DIR_LstAccDate[0] = 0;
	short_dir.DIR_LstAccDate[1] = 0;
	short_dir.DIR_FstClusHI[0] = (uint8_t)(cluster >> 24);
	short_dir.DIR_FstClusHI[1] = (uint8_t)(cluster>>16);
	short_dir.DIR_WrtTime[0] = 0;
	short_dir.DIR_WrtTime[1] = 0;
	short_dir.DIR_WrtDate[0] = 0;
	short_dir.DIR_WrtDate[1] = 0;
	short_dir.DIR_FstClusLO[0] = (uint8_t)cluster;
	short_dir.DIR_FstClusLO[1] = (uint8_t)(cluster >> 8);
	short_dir.DIR_FileSize[0] = (uint8_t)f_size;
	short_dir.DIR_FileSize[1] = (uint8_t)(f_size >> 8);
	short_dir.DIR_FileSize[2] = (uint8_t)(f_size >> 16);
	short_dir.DIR_FileSize[3] = (uint8_t)(f_size >> 24);

	long_dir.LDIR_Ord = 0x40 | 1;
	uint32_t j = 0;
	for(uint32_t i = 0; i < 10; i+=2, j++) {
		long_dir.LDIR_Name1[i] = (j < size) ? filename[j] : 0;
		long_dir.LDIR_Name1[i+1] = 0;
	}
	long_dir.LDIR_Attr = ATTR_LONG_NAME;
	long_dir.LDIR_Type = 1;	
	long_dir.LDIR_Chksum = 0;
	for(uint32_t i = 0; i < 12; i+=2, ++j) {
		long_dir.LDIR_Name2[i] = (j < size) ? filename[j] : 0;
		long_dir.LDIR_Name2[i+1] = 0;
	}
	long_dir.LDIR_FstClusLO[0] = 0;
	long_dir.LDIR_FstClusLO[1] = 0;
	for(uint32_t i = 0; i < 4; i+=2, ++j) {
		long_dir.LDIR_Name3[i] = (j < size) ? filename[j] : 0;
		long_dir.LDIR_Name3[i+1] = 0;
	}

	fat_cache[cluster] = FAT_EOC;
	sd_write_block(64, (uint8_t*)fat_cache);
	fat_cache_fat(buff);

	uint32_t sector = calc_first_sector(current_dir.cluster);
	sd_read_block(sector, buff);
	uint32_t idx;
	for(idx = 0; idx < 512 && !(is_empty(buff[idx])); idx+= 32);
	for(uint32_t i = 0; i < 32; ++i)
		buff[idx+i] = ((uint8_t*)&long_dir)[i];
	for(uint32_t i = 0; i < 32; ++i) {
		buff[idx+i+32] = ((uint8_t*)&short_dir)[i];
	}
	sd_write_block(sector, buff);
}

// TODO
void fat_write_file(uint8_t* buff, uint32_t cluster) {
	uint32_t sector = calc_first_sector(cluster);
	sd_write_block(sector, buff);

	fat_cache[cluster] = FAT_EOC;
	sd_write_block(64, (uint8_t*)fat_cache);
	fat_cache_fat(buff);
}

void fat_delete_file(uint8_t* filename, uint32_t name1_size, uint8_t* buff) {
	uint8_t filename_copy[16] = {0};
	for(uint32_t i = 0; i < name1_size; i++)
		filename_copy[i] = filename[i];

	uint32_t i = fat_find_entry_idx(filename_copy, name1_size, current_dir.cluster, buff);
	if (i == 512) {
		printf("file '%s' not found\n", filename_copy);
		return;
	}
	if (is_dir(short_dir.DIR_Attr, 0)) {
		printf("'%s' is directory (use `rmdir` instead)\n", filename_copy);
		return;
	}

	file_t file;
	read_file(&file, &long_dir, &short_dir, buff, i);
	buff[i] = 0xe5;
	if (is_long_name(buff[i+11]))
		buff[i+32] = 0xe5;
	sd_write_block(calc_first_sector(current_dir.cluster), buff);
	if (file.cluster > 2) {
		fat_cache[file.cluster] = 0;
		sd_write_block(64, (uint8_t*)fat_cache);
	}
}

// TODO: much better than it was, but can probably be better
void fat_delete_dir(uint8_t* dirname, uint32_t name1_size, uint8_t* buff) {
	uint8_t dirname_copy[16] = {0};
	for(uint32_t i = 0; i < name1_size; i++)
		dirname_copy[i] = dirname[i];

	uint32_t i = fat_find_entry_idx(dirname_copy, name1_size, current_dir.cluster, buff);
	if (is_file(short_dir.DIR_Attr, 0)) {
		printf("'%s' is file (use `rm` instead)\n", dirname_copy);
		return;
	}
	if (i == 512) {
		printf("directory '%s' not found\n", dirname_copy);
		return;
	}

	directory_t directory;
	read_dir(&directory, &long_dir, &short_dir, buff, i);

	// recursive delete of directory
	sd_read_block(calc_first_sector(directory.cluster), buff);
	for (uint32_t k = 0; k < 512; k += (!is_long_name(buff[k+11]) ? 32 : 64)) {
		if(is_empty(buff[k])) {
			continue;
		}

		if (is_dir(buff[k+11], buff[k+11+32])) {
			directory_t directory_in;
			read_dir(&directory_in, &long_dir, &short_dir, buff, k);
			uint32_t size = 0;
			for(uint32_t j = 0; j < 16 && directory_in.name[j] != 0; j++, size++);
			if (str_eq(directory_in.name, size, (uint8_t*)"..         ", 11) || str_eq(directory_in.name, size, (uint8_t*)".          ", 11)) {
				continue;
			}
			// works, but has to re-search for directory even though knows it exists
			uint32_t cluster_backup = current_dir.cluster;
			current_dir.cluster = directory.cluster;
			fat_delete_dir(directory_in.name, size, buff);
			current_dir.cluster = cluster_backup;
		} else if (is_file(buff[k+11], buff[k+11+32])) {
			file_t file;
			read_file(&file, &long_dir, &short_dir, buff, k);
			sd_read_block(calc_first_sector(directory.cluster), buff);
			buff[k] = 0xe5;
			if (is_long_name(buff[k+11]))
				buff[k+32] = 0xe5;
			sd_write_block(calc_first_sector(directory.cluster), buff);
			if (file.cluster > 2) {
				fat_cache[file.cluster] = 0;
				sd_write_block(64, (uint8_t*)fat_cache);
			}
		}
	}

	sd_read_block(calc_first_sector(current_dir.cluster), buff);
	buff[i] = 0xe5;
	if (is_long_name(buff[i+11]))
		buff[i+32] = 0xe5;
	sd_write_block(calc_first_sector(current_dir.cluster), buff);
	if (directory.cluster > 2) {
		fat_cache[directory.cluster] = 0;
		sd_write_block(64, (uint8_t*)fat_cache);
	}
}



void find_free_sector(void) {

}

void find_free_cluster(void) {

}

uint8_t calc_checksum(uint8_t name[]) {
	uint8_t sum = 0;
	// 11 == length of short dir name
	for (uint8_t i = 11; i > 0; --i)
		sum = (uint8_t)(((sum & 1) ? 0x80 : 0) + (sum >> 1) + name[11-i]);
	return sum;
}

void list_dir(uint8_t* buff) {
	uint32_t sector = calc_first_sector(current_dir.cluster);
	sd_read_block(sector, buff);

	for (uint32_t i = 0; i < 512; i += (!is_long_name(buff[i+11]) ? 32 : 64)) {
		if(is_empty(buff[i])) {
			continue;
		}

		if (is_dir(buff[i+11], buff[i+11+32])) {
			directory_t directory;
			read_dir(&directory, &long_dir, &short_dir, buff, i);
			printf("(dir) %s\n", directory.name);
		} else if (is_file(buff[i+11], buff[i+11+32])) {
			file_t file;
			read_file(&file, &long_dir, &short_dir, buff, i);
			printf("(file) %s -- %lu B\n", file.name, file.filesize);
		}
	}
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
		fat_set_dir_root(&current_dir);	
		return;
	}

	if (str_eq((uint8_t*)new_dir, name1_size, (uint8_t*)"..", 2)) {
		name1_size = 11;
		for(uint32_t i = 2; i < 11; ++i)
			new_dir[i] = ' ';
	}

	uint32_t i = fat_find_entry_idx(new_dir, name1_size, current_dir.cluster, buff);
	directory_t directory;
	read_dir(&directory, &long_dir, &short_dir, buff, i);
	current_dir.cluster = directory.cluster;
	for(uint32_t j = 0; j < 16; j++)
		current_dir.name[j] = directory.name[j];

	printf("directory '%s' not found\n", new_dir);
}

void cat_file(char* filename, uint8_t* buff) {
	uint8_t name1_size = 0;
	uint8_t file_name[16] = {0};
	for(uint32_t j = 0; j < 16 && filename[j] != 0; j++, name1_size++)
		file_name[j] = filename[j];

	uint32_t i = fat_find_entry_idx(file_name, name1_size, current_dir.cluster, buff);
	if (is_dir(short_dir.DIR_Attr, 0)) {
		printf("'%s' is a directory\n", file_name);
		return;
	}
	if (i == 512) {
		printf("file '%s' not found\n", file_name);
		return;
	}

	file_t file;
	read_file(&file, &long_dir, &short_dir, buff, i);
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

	// find free cluster
	uint32_t cluster = 2;
	for(; cluster < FAT_CACHE_CAPACITY && fat_cache[cluster]; ++cluster);

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
	printf("Dir: %s\n", current_dir.name);
}

void edit_file(uint8_t* tx_buff, uint8_t* rx_buff) {
	minied_main(tx_buff, rx_buff);
}


void make_dir(char* dirname, uint8_t* buff) {
	uint32_t size = 0;
	for(uint32_t j = 0; j < 16 && dirname[j] != 0; j++, size++);

	// find free cluster
	uint32_t cluster = 2;
	for(; cluster < FAT_CACHE_CAPACITY && fat_cache[cluster]; ++cluster);

	fat_create_file((uint8_t*)dirname, size, ATTR_DIRECTORY, cluster, 0, buff);

	// create the '.' and '..' directories
	fat_create_curr_dir(cluster, buff);
	fat_create_prev_dir(cluster, buff);
	
}

void fat_create_curr_dir(uint32_t cluster, uint8_t* buff) {
	short_dir.DIR_Name[0] = '.';
	for(uint32_t i = 1; i < 11; ++i)
		short_dir.DIR_Name[i] = ' ';
	short_dir.DIR_Attr = ATTR_DIRECTORY;
	short_dir.DIR_NTres = 0;
	short_dir.DIR_CrtTimeTenth = 0;
	short_dir.DIR_CrtTime[0] = 0;
	short_dir.DIR_CrtTime[1] = 0;
	short_dir.DIR_CrtDate[0] = 0;
	short_dir.DIR_CrtDate[1] = 0;
	short_dir.DIR_LstAccDate[0] = 0;
	short_dir.DIR_LstAccDate[1] = 0;
	short_dir.DIR_FstClusHI[0] = (uint8_t)(cluster >> 24);
	short_dir.DIR_FstClusHI[1] = (uint8_t)(cluster>>16);
	short_dir.DIR_WrtTime[0] = 0;
	short_dir.DIR_WrtTime[1] = 0;
	short_dir.DIR_WrtDate[0] = 0;
	short_dir.DIR_WrtDate[1] = 0;
	short_dir.DIR_FstClusLO[0] = (uint8_t)cluster;
	short_dir.DIR_FstClusLO[1] = (uint8_t)(cluster >> 8);
	short_dir.DIR_FileSize[0] = 0;
	short_dir.DIR_FileSize[1] = 0;
	short_dir.DIR_FileSize[2] = 0;
	short_dir.DIR_FileSize[3] = 0;

	sd_read_block(calc_first_sector(cluster), buff);
	for(uint32_t i = 0; i < 32; ++i)
		buff[i] = ((uint8_t*)&short_dir)[i];
	sd_write_block(calc_first_sector(cluster), buff);
}

void fat_create_prev_dir(uint32_t cluster, uint8_t* buff) {
	short_dir.DIR_Name[0] = '.';
	short_dir.DIR_Name[1] = '.';
	for(uint32_t i = 2; i < 11; ++i)
		short_dir.DIR_Name[i] = ' ';
	short_dir.DIR_Attr = ATTR_DIRECTORY;
	short_dir.DIR_NTres = 0;
	short_dir.DIR_CrtTimeTenth = 0;
	short_dir.DIR_CrtTime[0] = 0;
	short_dir.DIR_CrtTime[1] = 0;
	short_dir.DIR_CrtDate[0] = 0;
	short_dir.DIR_CrtDate[1] = 0;
	short_dir.DIR_LstAccDate[0] = 0;
	short_dir.DIR_LstAccDate[1] = 0;
	short_dir.DIR_FstClusHI[0] = (uint8_t)(current_dir.cluster >> 24);
	short_dir.DIR_FstClusHI[1] = (uint8_t)(current_dir.cluster >> 16);
	short_dir.DIR_WrtTime[0] = 0;
	short_dir.DIR_WrtTime[1] = 0;
	short_dir.DIR_WrtDate[0] = 0;
	short_dir.DIR_WrtDate[1] = 0;
	short_dir.DIR_FstClusLO[0] = (uint8_t)current_dir.cluster;
	short_dir.DIR_FstClusLO[1] = (uint8_t)(current_dir.cluster >> 8);
	short_dir.DIR_FileSize[0] = 0;
	short_dir.DIR_FileSize[1] = 0;
	short_dir.DIR_FileSize[2] = 0;
	short_dir.DIR_FileSize[3] = 0;

	sd_read_block(calc_first_sector(cluster), buff);
	for(uint32_t i = 32; i < 64; ++i)
		buff[i] = ((uint8_t*)&short_dir)[i-32];
	sd_write_block(calc_first_sector(cluster), buff);
}

uint32_t calc_first_sector(uint32_t cluster) {
	if (cluster == 0)
		return 0;
	if (cluster == 1)
		return 64;

	return (cluster-2)*bpb_SecPerClus + FirstDataSector;
}


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

				// find free cluster
				uint32_t cluster = 2;
				for(; cluster < FAT_CACHE_CAPACITY && fat_cache[cluster]; ++cluster);

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