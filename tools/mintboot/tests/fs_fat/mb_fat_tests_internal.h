#ifndef MINTBOOT_MB_FAT_TESTS_INTERNAL_H
#define MINTBOOT_MB_FAT_TESTS_INTERNAL_H

#include "mintboot/mb_portable.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_osbind.h"
#include "../../portable/fs_fat/mb_fat_internal.h"

struct mb_test_dta {
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

struct mb_test_bpb {
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

struct mb_test_dir_snap {
	uint32_t start_cluster;
	uint32_t sectors;
	uint32_t buf_offset;
};

struct mb_test_snap {
	uint16_t dev;
	uint16_t recsiz;
	uint16_t spc;
	uint16_t num_fats;
	uint16_t fat_sectors;
	uint32_t fat_start;
	uint32_t root_start;
	uint32_t root_sectors;
	uint16_t dir_count;
	uint32_t dir_used;
	struct mb_test_dir_snap dirs[8];
	uint8_t fat_buf[512 * 512];
	uint8_t root_buf[512 * 512];
	uint8_t dir_buf[256 * 512];
};

struct mb_fat_test_ctx {
	char spec[32];
	char missing[32];
	char missing_dir[40];
	char missing_dir_spec[40];
	char wrong_drive[40];
	char inner_path[40];
	char inner_missing[40];
	char inner_spec[40];
	char rename_src[40];
	char rename_dst[40];
	char move_src[40];
	char move_dst[40];
	char rename_dir_src[40];
	char rename_dir_dst[40];
	char missing_rename[40];
	char missing_dir_rename[40];
	char missing_dst_dir[40];
	char wrong_drive_rename[40];
	char renamed_spec[40];
	char moved_spec[40];
	char newdir_spec[40];
	char dcreate_missing_parent[40];
	char dcreate_root[40];
	char dcreate_exist[40];
	char ddelete_dir[40];
	char ddelete_missing[40];
	char ddelete_file[40];
	char ddelete_missing_dir[40];
	char ddelete_nonempty[40];
	char rename_exist_src[40];
	char rename_exist_dst[40];
	char rename_readonly_src[40];
	char rename_readonly_dst[40];
	char rename_readonly_target_src[40];
	char rename_readonly_target_dst[40];
	char wildcard[32];
	char noslash[32];
	char mixed[40];

	uint8_t buf[128];
	long handles[MB_FAT_MAX_OPEN];
	uint16_t dfree[4];
	uint16_t timebuf[2];
	uint16_t timebuf2[2];
	struct mb_test_dta dta;
	struct mb_test_dta dta2;
	struct mb_fat_check_report report;
};

extern struct mb_test_snap mb_test_snap;
extern uint16_t mb_tests_drive_dev;
extern char mb_tests_drive_letter;

int mb_tests_strcmp(const char *a, const char *b);
int mb_tests_memcmp(const uint8_t *a, const uint8_t *b, uint32_t len);
long mb_tests_fsfirst(const char *spec, uint16_t attr, struct mb_test_dta *dta);
void mb_tests_drain_search(void);
void mb_tests_init_drive(void);
void mb_tests_make_path(char *out, uint32_t outsz, char drive, const char *suffix);
void mb_tests_make_noslash_path(char *out, uint32_t outsz, char drive, const char *suffix);
const struct mb_test_bpb *mb_tests_getbpb(void);
void mb_tests_snapshot_fs(uint16_t dev);
void mb_tests_restore_fs(void);
void mb_tests_set_readonly(const char name83[11]);
void mb_fat_tests_init_context(struct mb_fat_test_ctx *t);

void mb_fat_tests_phase_lookup_io(struct mb_fat_test_ctx *t);
void mb_fat_tests_phase_rename_attr_seek(struct mb_fat_test_ctx *t);
void mb_fat_tests_phase_dir_integrity(struct mb_fat_test_ctx *t);

#endif
