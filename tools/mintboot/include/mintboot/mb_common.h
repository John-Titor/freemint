#ifndef MINTBOOT_MB_COMMON_H
#define MINTBOOT_MB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#if defined(MB_ENABLE_COVERAGE) && MB_ENABLE_COVERAGE && defined(__GNUC__)
#define MB_COV_EXCLUDE __attribute__((no_profile_instrument_function))
#else
#define MB_COV_EXCLUDE
#endif

/* Marks code paths that are intentionally excluded from coverage accounting. */
#define MB_COV_EXPECT_UNHIT MB_COV_EXCLUDE

#define NORETURN __attribute__((noreturn))
#define INTERRUPT __attribute__((interrupt))

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
void mb_common_set_tt_ram(uint32_t base, uint32_t size);
void mb_coverage_dump(void);

uint16_t mb_common_boot_drive(void);

/* Vector/trap setup helpers. */
void mb_common_setup_vectors(void);
uint32_t mb_common_vector_base(void);

/* Minimal logging helpers (common layer). */
void mb_log_puts(const char *s);
void mb_log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void mb_log_vprintf(const char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));
void mb_panic(const char *fmt, ...) __attribute__((format(printf, 1, 2))) NORETURN;
void mb_panic2(const char *fmt, ...) __attribute__((format(printf, 1, 2))) NORETURN;

#endif /* MINTBOOT_MB_COMMON_H */
