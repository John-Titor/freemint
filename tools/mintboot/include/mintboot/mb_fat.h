#ifndef MINTBOOT_MB_FAT_H
#define MINTBOOT_MB_FAT_H

#include <stdint.h>

long mb_fat_fopen(const char *path, uint16_t mode);
long mb_fat_fcreate(const char *path, uint16_t mode);
long mb_fat_fread(uint16_t handle, uint32_t cnt, void *buf);
long mb_fat_fwrite(uint16_t handle, uint32_t cnt, void *buf);
long mb_fat_fclose(uint16_t handle);
long mb_fat_fdelete(const char *path);
long mb_fat_fseek(uint16_t handle, int32_t where, uint16_t how);
long mb_fat_dfree(uint32_t buf, uint16_t d);
struct mb_fat_check_report {
	uint32_t bad_bpb;
	uint32_t bad_dirent;
	uint32_t bad_chain;
	uint32_t bad_fat;
	uint32_t lost_clusters;
	uint32_t cross_links;
};

long mb_fat_check(uint16_t dev, struct mb_fat_check_report *report);

#endif /* MINTBOOT_MB_FAT_H */
