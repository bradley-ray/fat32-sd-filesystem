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

void read_dir(directory_t* dir_handle, uint8_t* buff, uint32_t idx);
void read_file(file_t* file_handle, uint8_t* buff, uint32_t idx);

// FAT functions and variables
uint32_t calc_first_sector(uint32_t cluster);
uint8_t calc_checksum(uint8_t name[]);
uint8_t calc_longname(void);
uint8_t calc_shortname(void);

// FAT32 HAL functions
void fat_write_block(uint32_t addr, uint8_t* buff);
void fat_read_block(uint32_t addr, uint8_t* buff);

// FAT32 main functions
uint32_t fat_find_entry_idx(uint8_t* entry_name, uint32_t size, uint32_t cluster, uint8_t* buff);
void fat_init(uint8_t* buff);
void fat_read_boot(uint8_t* buff);
void fat_get_clus_info(uint8_t* buff);
void fat_set_dir_root(void);
void fat_open_dir(directory_t* directory);
void fat_cache_fat(uint8_t* buff);
void fat_create_file(uint8_t* filename, uint32_t size, uint8_t type, uint32_t cluster, uint32_t f_size, uint8_t* buff);
void fat_write_file(uint8_t* buff, uint32_t f_size);
void fat_create_curr_dir(uint32_t cluster, uint8_t* buff);
void fat_create_prev_dir(uint32_t cluster, uint8_t* buff);
uint32_t fat_find_free_cluster(void);
uint32_t fat_get_current_dir_cluster(void);
uint8_t* fat_get_current_dir_name(void);
void fat_update_current_dir(directory_t* dir);
void fat_delete_dir(uint8_t* dirname, uint32_t name1_size, uint8_t* buff);
void fat_delete_file(uint8_t* filename, uint32_t name1_size, uint8_t* buff);
uint32_t fat_find_next_dir_entry(directory_t* dir, uint32_t cluster, uint32_t start, uint8_t* buff);
uint32_t fat_find_next_file_entry(file_t* file, uint32_t cluster, uint32_t start, uint8_t* buff);

#endif