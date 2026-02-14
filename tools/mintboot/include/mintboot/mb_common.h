#ifndef MINTBOOT_MB_COMMON_H
#define MINTBOOT_MB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

extern char mb_cmdline[128];

/* Common boot flow entry point (called from board layer). */
void mb_common_start(void);

/* Common kernel loader. */
void mb_common_boot(void);
int mb_common_load_kernel(const char *path, int do_jump);
int mb_common_find_kernel_path(char *out, size_t outsz);
uint32_t mb_common_last_basepage(void);
void mb_common_kernel_bounds(uint32_t *base, uint32_t *end);
void mb_common_set_st_ram(uint32_t base, uint32_t size);
void mb_coverage_dump(void);

uint16_t mb_common_boot_drive(void);

/* Vector/trap setup helpers. */
void mb_common_setup_vectors(void);
uint32_t mb_common_vector_base(void);

/* Minimal logging helpers (common layer). */
void mb_log_puts(const char *s);
void mb_log_hex32(uint32_t value);
void mb_log_hex64(uint64_t value);
void mb_log_u32(uint32_t value);
void mb_log_i32(int32_t value);
void mb_log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void mb_log_vprintf(const char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));
void mb_panic(const char *fmt, ...) __attribute__((format(printf, 1, 2))) __attribute__((noreturn));

#endif /* MINTBOOT_MB_COMMON_H */
