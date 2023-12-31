#ifndef __FAT32_h__
#define __FAT32_h__

#define FAT_CACHE_CAPACITY 128

#include <inttypes.h>

/*
 * TODO
 *      - need to cache of files
 *      - need a struct for a directory
 *      - need struct of current directory
 *      - need cache of FAT table
 *      - need to be able to write files/dirs
 */
#define ROOT_CLUSTER 2

#define FAT_EOF 0x8fffffff
#define FAT_EOC 0x0fffffff

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)


typedef struct {
    uint8_t LDIR_Ord;
    uint8_t LDIR_Name1[10];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint8_t LDIR_Name2[12];
    uint8_t LDIR_FstClusLO[2];
    uint8_t LDIR_Name3[4];
} ldir_entry_t;

typedef struct {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTres;
    uint8_t DIR_CrtTimeTenth;
    uint8_t DIR_CrtTime[2];
    uint8_t DIR_CrtDate[2];
    uint8_t DIR_LstAccDate[2];
    uint8_t DIR_FstClusHI[2];
    uint8_t DIR_WrtTime[2];
    uint8_t DIR_WrtDate[2];
    uint8_t DIR_FstClusLO[2];
    uint8_t DIR_FileSize[4];
} sdir_entry_t;

typedef struct {
    uint32_t cluster;
    uint8_t name[16];
} directory_t;

typedef struct {
    uint32_t filesize;
    uint32_t cluster;
    uint8_t name[16];
} file_t;

uint8_t is_long_name(uint8_t attr);

void read_dir(directory_t* dir_handle, ldir_entry_t* ldir_handle, sdir_entry_t* sdir_handle, uint8_t* buff, uint32_t idx);
void read_file(file_t* file_handle, ldir_entry_t* ldir_handle, sdir_entry_t* sdir_handle, uint8_t* buff, uint32_t idx);

// FAT functions and variables
uint32_t calc_first_sector(uint32_t cluster);
uint8_t calc_checksum(uint8_t name[]);
uint8_t calc_longname(void);
uint8_t calc_shortname(void);

void fat_init(uint8_t* buff);
void fat_read_boot(uint8_t* buff);
void fat_get_clus_info(uint8_t* buff);
void fat_set_dir_root(directory_t* directory);
void fat_open_dir(directory_t* directory);
void fat_cache_fat(uint8_t* buff);
void fat_create_file(uint8_t* filename, uint32_t size, uint8_t type, uint32_t cluster, uint32_t f_size, uint8_t* buff);
void fat_write_file(uint8_t* buff, uint32_t f_size);
void fat_create_curr_dir(uint32_t cluster, uint8_t* buff);
void fat_create_prev_dir(uint32_t cluster, uint8_t* buff);

void list_dir(uint8_t* buff);
void change_dir(char* dirname, uint8_t* buff);
void cat_file(char* filename, uint8_t* buff);
void print_current_dir(void);
void edit_file(uint8_t* tx_buff, uint8_t* rx_buff);
void create_file(char* filename, uint8_t* buff);
void delete_file(char* filename, uint8_t* buff);
void make_dir(char* dirname, uint8_t* buff);
void delete_dir(char* dirname, uint8_t* buff);

// MiniEditor functions and buffer
void minied_main(uint8_t* tx_buff, uint8_t* rx_buff);
typedef struct {
	uint8_t line1[128];
	uint8_t line2[128];
	uint8_t line3[128];
	uint8_t line4[128];
} minied_buff_t;

#endif