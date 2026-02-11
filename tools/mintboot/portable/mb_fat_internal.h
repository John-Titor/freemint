#ifndef MINTBOOT_MB_FAT_INTERNAL_H
#define MINTBOOT_MB_FAT_INTERNAL_H

#include "mintboot/mb_fat.h"
#include "mintboot/mb_errors.h"

#include <stdint.h>

#define MB_FAT_MAX_SEARCH 4
#define MB_FAT_MAX_VOLS 8

#define MB_FAT_ATTR_RDONLY 0x01
#define MB_FAT_ATTR_HIDDEN 0x02
#define MB_FAT_ATTR_SYSTEM 0x04
#define MB_FAT_ATTR_LABEL  0x08
#define MB_FAT_ATTR_DIR    0x10
#define MB_FAT_ATTR_LFN    0x0f

#define MB_FAT_DTA_VALID 0x1234fedcUL

struct mb_fat_bpb {
	uint16_t recsiz;
	int16_t clsiz;
	uint16_t clsizb;
	int16_t rdlen;
	int16_t fsiz;
	int16_t fatrec;
	int16_t datrec;
	uint16_t numcl;
	int16_t bflags;
};

struct mb_fat_volume {
	uint8_t valid;
	uint8_t dev;
	uint16_t recsiz;
	uint16_t spc;
	uint16_t num_fats;
	uint16_t fat_sectors;
	uint32_t fat_start;
	uint32_t root_start;
	uint32_t root_sectors;
	uint32_t data_start;
	uint32_t total_clusters;
};

struct mb_fat_dirent {
	uint8_t name[11];
	uint8_t attr;
	uint8_t ntres;
	uint8_t crt_tenths;
	uint16_t crt_time;
	uint16_t crt_date;
	uint16_t acc_date;
	uint16_t start_hi;
	uint16_t wr_time;
	uint16_t wr_date;
	uint16_t start_lo;
	uint32_t size;
} __attribute__((packed));

struct mb_fat_search {
	uint8_t in_use;
	uint8_t dev;
	uint8_t pattern[11];
	uint16_t attr;
	uint32_t dir_cluster;
	uint32_t entry_index;
};

struct mb_fat_open {
	uint8_t in_use;
	uint8_t dev;
	uint32_t size;
	uint32_t start_cluster;
	uint32_t offset;
	uint16_t time;
	uint16_t date;
	uint8_t attr;
};

struct mb_fat_dta {
	uint16_t index;
	uint32_t magic;
	char dta_pat[14];
	char dta_sattrib;
	char dta_attrib;
	uint16_t dta_time;
	uint16_t dta_date;
	uint32_t dta_size;
	char dta_name[14];
};

extern struct mb_fat_volume *mb_fat_vol;
extern struct mb_fat_volume mb_fat_vols[MB_FAT_MAX_VOLS];
extern struct mb_fat_search mb_fat_search[MB_FAT_MAX_SEARCH];
extern struct mb_fat_open mb_fat_open[MB_FAT_MAX_OPEN];

uint16_t mb_fat_le16(const uint8_t *ptr);
uint32_t mb_fat_le32(const uint8_t *ptr);
int mb_fat_rwabs(uint16_t rw, void *buf, uint16_t count, uint32_t recno,
		 uint16_t dev);
int mb_fat_mount(uint16_t dev);
uint32_t mb_fat_cluster_sector(uint32_t cluster);
uint16_t mb_fat_read_fat(uint32_t cluster);
int mb_fat_write_fat(uint32_t cluster, uint16_t value);
uint32_t mb_fat_alloc_cluster(void);
int mb_fat_free_chain(uint32_t start_cluster);
int mb_fat_zero_cluster(uint32_t cluster);
void mb_fat_pattern_83(const char *pattern, uint8_t out[11]);
int mb_fat_match_83(const uint8_t name[11], const uint8_t pat[11]);
void mb_fat_name_from_dirent(const struct mb_fat_dirent *ent, char out[14]);
int mb_fat_has_wildcards(const char *name);
int mb_fat_dirent_location(uint32_t dir_cluster, uint32_t index,
			  uint32_t *sector_idx, uint32_t *sector_off);
int mb_fat_read_dirent_raw(uint32_t dir_cluster, uint32_t index,
			  uint8_t raw[32]);
void mb_fat_decode_dirent(const uint8_t raw[32], struct mb_fat_dirent *out);
int mb_fat_write_dirent_raw(uint32_t dir_cluster, uint32_t index,
			   const uint8_t raw[32]);
int mb_fat_read_dirent(uint32_t dir_cluster, uint32_t index,
		       struct mb_fat_dirent *out);
int mb_fat_find_in_dir(uint32_t dir_cluster, const uint8_t pat[11],
		      uint16_t attr, struct mb_fat_dirent *ent, uint32_t *index);
int mb_fat_dir_is_empty(uint32_t dir_cluster);
int mb_fat_find_path(uint16_t dev, const char *path, struct mb_fat_dirent *ent);
int mb_fat_setup_search(const char *filespec, uint16_t attr,
			struct mb_fat_search **out);
int mb_fat_locate_parent(const char *path, uint16_t *dev_out,
			  uint32_t *dir_cluster_out, uint8_t name83[11]);
int mb_fat_parse_drive(const char *path, uint16_t *dev_out);
int mb_fat_find_free_entry(uint32_t dir_cluster, uint32_t *index);
struct mb_fat_open *mb_fat_get_open(uint16_t handle);
int mb_fat_cluster_for_offset(const struct mb_fat_open *op, uint32_t offset,
			      uint32_t *cluster_out, uint32_t *cluster_off_out);

#endif /* MINTBOOT_MB_FAT_INTERNAL_H */
