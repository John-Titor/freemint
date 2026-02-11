#include "mintboot/mb_portable.h"
#include "mintboot/mb_tests.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_cookie.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_rom.h"

#ifndef str
#define str(x) _stringify(x)
#define _stringify(x) #x
#endif
#include "../../../sys/buildinfo/version.h"

#include <stddef.h>

extern void mb_install_vector_base(void);

struct mb_cookie_jar mb_cookie_jar;
char mb_cmdline[128];
static uint16_t mb_boot_drive = 0xffffu;

#define MB_COOKIE_PTR_ADDR      0x000005a0u
#define MB_COOKIE_STORAGE_ADDR  0x00000600u
#define MB_COOKIE_CAPACITY      16u

static void mb_portable_init_cookies(void)
{
	struct mb_cookie *storage = (struct mb_cookie *)MB_COOKIE_STORAGE_ADDR;
	volatile struct mb_cookie **jar_ptr = (volatile struct mb_cookie **)MB_COOKIE_PTR_ADDR;

	mb_cookie_init(&mb_cookie_jar, storage, MB_COOKIE_CAPACITY);
	*jar_ptr = mb_cookie_entries(&mb_cookie_jar);

	/* Defaults; board code may override. */
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'C', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'F', 'P', 'U'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'M', 'C', 'H'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'V', 'D', 'O'), 0xffffffffu);
	mb_cookie_set(&mb_cookie_jar, MB_COOKIE_ID('_', 'S', 'N', 'D'), 0xffffffffu);
}

static void mb_etv_critic_stub(void)
{
}

static void mb_etv_term_stub(void)
{
}

static void mb_etv_timer_stub(void)
{
	(*mb_lm_vbclock())++;
}

static void mb_portable_init_lowmem(void)
{
	*mb_lm_etv_critic() = (uint32_t)(uintptr_t)mb_etv_critic_stub;
	*mb_lm_etv_term() = (uint32_t)(uintptr_t)mb_etv_term_stub;
	*mb_lm_etv_timer() = (uint32_t)(uintptr_t)mb_etv_timer_stub;
	*mb_lm_vbclock() = 0;
}

__attribute__((weak)) void mb_board_init_cookies(void)
{
}

__attribute__((weak)) uint32_t mb_board_kernel_tpa_start(void)
{
	return 0;
}

static void mb_portable_init_boot_drive(void)
{
	uint32_t map = (uint32_t)mb_rom_dispatch.drvmap();
	uint16_t i;

	mb_boot_drive = 0xffffu;
	if (map == 0)
		return;

	for (i = 0; i < 26; i++) {
		if (map & (1u << i)) {
			mb_boot_drive = i;
			return;
		}
	}
}

uint16_t mb_portable_boot_drive(void)
{
	return mb_boot_drive;
}

static void mb_strlcpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;

	if (!n)
		return;

	for (i = 0; i + 1 < n && src[i]; i++)
		dst[i] = src[i];
	dst[i] = '\0';
}

static void mb_strlcat(char *dst, const char *src, size_t n)
{
	size_t dlen = 0;
	size_t i;

	if (!n)
		return;

	while (dlen < n && dst[dlen])
		dlen++;

	if (dlen == n)
		return;

	for (i = 0; dlen + i + 1 < n && src[i]; i++)
		dst[dlen + i] = src[i];
	dst[dlen + i] = '\0';
}

static int mb_path_exists(const char *path)
{
	long fh;

	fh = mb_rom_fopen(path, 0);
	if (fh < 0)
		return 0;
	mb_rom_fclose((uint16_t)fh);
	return 1;
}

static int mb_find_boot_drive(char *drive_out)
{
	if (mb_boot_drive >= 26)
		return -1;
	*drive_out = (char)('A' + mb_boot_drive);
	return 0;
}

static int mb_find_kernel_path(char *out, size_t outsz)
{
	const char *name = "mint000.prg";
	uint32_t cpu;
	char drive;
	char prefix[4];

	if (mb_find_boot_drive(&drive) != 0)
		return -1;
	prefix[0] = drive;
	prefix[1] = ':';
	prefix[2] = '\\';
	prefix[3] = '\0';

	mb_strlcpy(out, prefix, outsz);
	mb_strlcat(out, "MINT\\", outsz);
	mb_strlcat(out, MINT_VERS_PATH_STRING, outsz);
	mb_strlcat(out, "\\", outsz);
	mb_strlcat(out, name, outsz);
	if (mb_path_exists(out))
		return 0;

	if (mb_cookie_get(&mb_cookie_jar, MB_COOKIE_ID('_', 'C', 'P', 'U'), &cpu) == 0) {
		switch (cpu) {
		case 20:
			name = "mint020.prg";
			break;
		case 30:
			name = "mint030.prg";
			break;
		case 40:
			name = "mint040.prg";
			break;
		case 60:
			name = "mint060.prg";
			break;
		default:
			break;
		}
	}

	mb_strlcpy(out, prefix, outsz);
	mb_strlcat(out, "MINT\\", outsz);
	mb_strlcat(out, MINT_VERS_PATH_STRING, outsz);
	mb_strlcat(out, "\\", outsz);
	mb_strlcat(out, name, outsz);
	if (mb_path_exists(out))
		return 0;

	return -1;
}

void mb_portable_setup_vectors(void)
{
	mb_install_vector_base();
}

void mb_portable_setup_traps(void)
{
	/* TODO: install ROM_* trap emulation handlers. */
}

void mb_portable_run_tests(void);

void mb_portable_boot(struct mb_boot_info *info)
{
	(void)info;

	mb_cmdline[0] = '\0';
	mb_portable_init_lowmem();
	mb_portable_init_cookies();
	mb_board_init_cookies();
	mb_portable_init_boot_drive();
	mb_portable_setup_vectors();
	mb_portable_setup_traps();
	mb_portable_run_tests();
	{
		char kernel_path[384];
		if (mb_find_kernel_path(kernel_path, sizeof(kernel_path)) == 0) {
			mb_log_printf("mintboot: kernel candidate %s\r\n", kernel_path);
			if (mb_portable_load_kernel(kernel_path, 0) != 0)
				mb_log_puts("mintboot: kernel load failed\r\n");
			else
				mb_log_puts("mintboot: kernel loaded (no jump)\r\n");
		} else {
			mb_log_puts("mintboot: kernel not found\r\n");
		}
	}
	mb_log_puts("mintboot: portable init complete\r\n");
	/* TODO: load/relocate kernel, finalize boot info, jump to entry. */
}
