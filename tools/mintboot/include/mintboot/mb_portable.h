#ifndef MINTBOOT_MB_PORTABLE_H
#define MINTBOOT_MB_PORTABLE_H

#include <stdint.h>
#include <stdarg.h>

struct mb_boot_info {
	uint32_t magic;
	void *kernel_entry;
	void *kernel_base;
	uint32_t kernel_size;
	void *ramdisk_base;
	uint32_t ramdisk_size;
};

extern char mb_cmdline[128];

/* Portable boot flow entry point (called from board layer). */
void mb_portable_boot(struct mb_boot_info *info);

/* Vector/trap setup helpers. */
void mb_portable_setup_vectors(void);
void mb_portable_setup_traps(void);

/* Minimal logging helpers (portable layer). */
void mb_log_puts(const char *s);
void mb_log_hex32(uint32_t value);
void mb_log_hex64(uint64_t value);
void mb_log_u32(uint32_t value);
void mb_log_i32(int32_t value);
void mb_log_printf(const char *fmt, ...);
void mb_log_vprintf(const char *fmt, va_list ap);
void mb_panic(const char *fmt, ...);

struct mb_exception_context *mb_last_exception_context(void);

void mb_portable_run_tests(void);
void mb_fat_run_tests(void);

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
void mb_trap13_handler(struct mb_exception_context *ctx);
void mb_trap14_handler(struct mb_exception_context *ctx);

/* IRQ handlers. */
void mb_spurious_irq_handler(void);
void mb_autovec_level1_handler(void);
void mb_autovec_level2_handler(void);
void mb_autovec_level3_handler(void);
void mb_autovec_level4_handler(void);
void mb_autovec_level5_handler(void);
void mb_autovec_level6_handler(void);
void mb_autovec_level7_handler(void);


#endif /* MINTBOOT_MB_PORTABLE_H */
