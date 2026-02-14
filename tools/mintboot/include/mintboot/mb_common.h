#ifndef MINTBOOT_MB_COMMON_H
#define MINTBOOT_MB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

extern char mb_cmdline[128];

/* Common boot flow entry point (called from board layer). */
void mb_common_start(void);

/* Common kernel loader. */
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
void mb_log_printf(const char *fmt, ...);
void mb_log_vprintf(const char *fmt, va_list ap);
void mb_panic(const char *fmt, ...);

struct mb_exception_context *mb_last_exception_context(void);
uint32_t mb_last_user_usp(void);
uint32_t mb_last_entry_usp(void);
uint32_t mb_last_entry_sp(void);


struct mb_exception_frame {
	uint16_t sr;
	uint32_t pc;
	uint16_t format;
} __attribute__((packed));

struct mb_exception_context {
	uint32_t sp;
	uint32_t d[8];
	uint32_t a[7];
	struct mb_exception_frame frame;
} __attribute__((packed));

/* Exception handler (receives pointer to exception frame). */
void mb_default_exception_handler(struct mb_exception_context *ctx);

/* Trap handlers. */
void mb_trap1_handler(struct mb_exception_context *ctx);
void mb_trap2_handler(struct mb_exception_context *ctx);
void mb_trap13_handler(struct mb_exception_context *ctx);
void mb_trap14_handler(struct mb_exception_context *ctx);
void mb_trace_exception_handler(struct mb_exception_context *ctx);
void mb_linea_exception_handler(struct mb_exception_context *ctx);
void mb_fatal_vector_handler(uint32_t vec, uint32_t sp, uint32_t usp);

/* IRQ handlers. */
void mb_spurious_irq_handler(void);
void mb_autovec_level1_handler(void);
void mb_autovec_level2_handler(void);
void mb_autovec_level3_handler(void);
void mb_autovec_level4_handler(void);
void mb_autovec_level5_handler(void);
void mb_autovec_level6_handler(void);
void mb_autovec_level7_handler(void);


#endif /* MINTBOOT_MB_COMMON_H */
