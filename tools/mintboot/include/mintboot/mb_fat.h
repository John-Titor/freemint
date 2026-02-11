#ifndef MINTBOOT_MB_FAT_H
#define MINTBOOT_MB_FAT_H

#include <stdint.h>

long mb_fat_fsfirst(const char *filespec, uint16_t attr);
long mb_fat_fsnext(void);
long mb_fat_fopen(const char *path, uint16_t mode);
long mb_fat_fread(uint16_t handle, uint32_t cnt, uint32_t buf);
long mb_fat_fclose(uint16_t handle);
long mb_fat_fseek(uint16_t handle, int32_t where, uint16_t how);
long mb_fat_fdatime(uint32_t timeptr, uint16_t handle, uint16_t rwflag);
long mb_fat_fattrib(const char *fn, uint16_t rwflag, uint16_t attr);

#endif /* MINTBOOT_MB_FAT_H */
