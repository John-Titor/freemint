#ifndef MINTBOOT_MB_TESTS_INTERNAL_H
#define MINTBOOT_MB_TESTS_INTERNAL_H

#include <stdint.h>

uint32_t mb_tests_strlen(const char *s);
void mb_tests_strcat(char *dst, uint32_t dstsz, const char *src);

void mb_tests_setexc(void);
void mb_tests_gettime(void);
void mb_tests_drive_range(void);
void mb_tests_drive_path(void);
void mb_tests_bconmap(void);
void mb_tests_vbclock(void);
void mb_tests_kernel_loader(void);

#endif /* MINTBOOT_MB_TESTS_INTERNAL_H */
