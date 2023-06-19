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


void read_dir(directory_t* dir_handle, uint8_t* buff, uint32_t idx) {
    for(uint32_t j = 0; j < 32; ++j) {
        ((uint8_t*)&long_dir)[j] = buff[j+idx];
	}

	for(uint32_t i = 0; i < 16; ++i) {
		dir_handle->name[i] = 0;
	}

    if (is_long_name(long_dir.LDIR_Attr)) {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)&short_dir)[j] = buff[j+idx+32];
        for(uint32_t j = 0, k = 0; j < 10; j+=2, k++)
            dir_handle->name[k] = long_dir.LDIR_Name1[j];
        for(uint32_t j = 0, k = 5; j < 12; j+=2, k++)
            dir_handle->name[k] = long_dir.LDIR_Name2[j];
        for(uint32_t j = 0, k = 11; j < 4; j+=2, k++)
            dir_handle->name[k] = long_dir.LDIR_Name3[j];
    } else {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)&short_dir)[j] = buff[j+idx];
        for(uint32_t j = 0; j < 11; ++j)
            dir_handle->name[j] = short_dir.DIR_Name[j];
    }
    
	dir_handle->cluster = from_little_endian_32(short_dir.DIR_FstClusLO[0], short_dir.DIR_FstClusLO[1], short_dir.DIR_FstClusHI[0], short_dir.DIR_FstClusHI[1]);
	// root cluster is cluster 2, but marked 0 in .. entries
    dir_handle->cluster = (dir_handle->cluster != 0) ? dir_handle->cluster : 2;
}

void read_file(file_t* file_handle, uint8_t* buff, uint32_t idx) {
    for(uint32_t j = 0; j < 32; ++j) {
        ((uint8_t*)&long_dir)[j] = buff[j+idx];
	}

	for(uint32_t i = 0; i < 16; ++i) {
		file_handle->name[i] = 0;
	}

    if (is_long_name(long_dir.LDIR_Attr)) {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)&short_dir)[j] = buff[j+idx+32];
        for(uint32_t j = 0, k = 0; j < 10; j+=2, k++)
            file_handle->name[k] = long_dir.LDIR_Name1[j];
        for(uint32_t j = 0, k = 5; j < 12; j+=2, k++)
            file_handle->name[k] = long_dir.LDIR_Name2[j];
        for(uint32_t j = 0, k = 11; j < 4; j+=2, k++)
            file_handle->name[k] = long_dir.LDIR_Name3[j];
    } else {
        for(uint32_t j = 0; j < 32; ++j)
            ((uint8_t*)&short_dir)[j] = buff[j+idx];
        for(uint32_t j = 0; j < 11; ++j)
            file_handle->name[j] = short_dir.DIR_Name[j];
    }
    
	file_handle->cluster = from_little_endian_32(short_dir.DIR_FstClusLO[0], short_dir.DIR_FstClusLO[1], short_dir.DIR_FstClusHI[0], short_dir.DIR_FstClusHI[1]);
	file_handle->filesize = from_little_endian_32(short_dir.DIR_FileSize[0], short_dir.DIR_FileSize[1], short_dir.DIR_FileSize[2], short_dir.DIR_FileSize[3]);
}

uint32_t fat_find_entry_idx(uint8_t* entry_name, uint32_t size, uint32_t cluster, uint8_t* buff) {
	sd_read_block(calc_first_sector(cluster), buff);

	for (uint32_t i = 0; i < 512; i += (!is_long_name(buff[i+11]) ? 32 : 64)) {
		if(is_empty(buff[i])) {
			continue;
		}

		if (is_dir(buff[i+11], buff[i+11+32])) {
			directory_t dir;
			read_dir(&dir, buff, i);
			uint8_t name2_size = 0;
			for(uint32_t j = 0; j < 16 && dir.name[j] != 0; j++, name2_size++);
			if (str_eq(entry_name, size, dir.name, name2_size))
				return i;
		} else if (is_file(buff[i+11], buff[i+11+32])) {
			file_t file;
			read_file(&file, buff, i);
			uint8_t name2_size = 0;
			for(uint32_t j = 0; j < 16 && file.name[j] != 0; j++, name2_size++);
			if (str_eq(entry_name, size, file.name, name2_size))
				return i;
		}
	}

	// NOT FOUND
	return 512;
}

uint32_t fat_find_next_dir_entry(directory_t* dir, uint32_t cluster, uint32_t start, uint8_t* buff) {
	sd_read_block(calc_first_sector(cluster), buff);

	start += !is_long_name(buff[start+11]) ? 32 : 64;
	for (uint32_t i = start; i < 512-32; i += (!is_long_name(buff[i+11]) ? 32 : 64)) {
		if(is_empty(buff[i])) {
			continue;
		}

		if (is_dir(buff[i+11], buff[i+11+32])) {
			read_dir(dir, buff, i);
			return i;
		}
	}

	return 512;
}

uint32_t fat_find_next_file_entry(file_t* file, uint32_t cluster, uint32_t start, uint8_t* buff) {
	sd_read_block(calc_first_sector(cluster), buff);

	start += !is_long_name(buff[start+11]) ? 32 : 64;
	for (uint32_t i = start; i < 512-32; i += (!is_long_name(buff[i+11]) ? 32 : 64)) {
		if(is_empty(buff[i])) {
			continue;
		}

		if (is_file(buff[i+11], buff[i+11+32])) {
			read_file(file, buff, i);
			return i;
		}
	}

	return 512;
}

uint32_t fat_get_current_dir_cluster(void) {
	return current_dir.cluster;
}

uint8_t* fat_get_current_dir_name(void) {
	return current_dir.name;
}

void fat_update_current_dir(directory_t* dir) {
	current_dir.cluster = dir->cluster;
	for(uint32_t j = 0; j < 16; j++)
		current_dir.name[j] = dir->name[j];
}

void fat_init(uint8_t* buff) {
	fat_read_boot(buff);
	fat_get_clus_info(buff);
	fat_cache_fat(buff);
	fat_set_dir_root();
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

void fat_set_dir_root(void) {
	current_dir.name[0] = 'r';
	current_dir.name[1] = 'o';
	current_dir.name[2] = 'o';
	current_dir.name[3] = 't';
	for(uint32_t i = 4; i < 11; i++)
		current_dir.name[i] = 0;

	current_dir.cluster = ROOT_CLUSTER;
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
	read_file(&file, buff, i);
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
	read_dir(&directory, buff, i);

	// recursive delete of directory
	sd_read_block(calc_first_sector(directory.cluster), buff);
	for (uint32_t k = 0; k < 512; k += (!is_long_name(buff[k+11]) ? 32 : 64)) {
		if(is_empty(buff[k])) {
			continue;
		}

		if (is_dir(buff[k+11], buff[k+11+32])) {
			directory_t directory_in;
			read_dir(&directory_in, buff, k);
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
			read_file(&file, buff, k);
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

void fat_find_free_sector(void) {
	// TODO
}

// TOOD: currently only looking at first sector of FAT
uint32_t fat_find_free_cluster(void) {
	uint32_t cluster = 2;
	for(; cluster < FAT_CACHE_CAPACITY && fat_cache[cluster]; ++cluster);
	return cluster;
}

uint8_t calc_checksum(uint8_t name[]) {
	uint8_t sum = 0;
	// 11 == length of short dir name
	for (uint8_t i = 11; i > 0; --i)
		sum = (uint8_t)(((sum & 1) ? 0x80 : 0) + (sum >> 1) + name[11-i]);
	return sum;
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