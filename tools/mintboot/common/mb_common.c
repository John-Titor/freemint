#include "mintboot/mb_common.h"
#include "mintboot/mb_tests.h"
#include "mintboot/mb_board.h"
#include "mintboot/mb_cookie.h"
#include "mintboot/mb_linea.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_cpu.h"

#include <stddef.h>

extern uint32_t mb_vector_table[];
extern uint8_t _mb_image_end[] __attribute__((weak));

char mb_cmdline[128];
static uint16_t mb_boot_drive = 0xffffu;

static void mb_etv_critic_stub(void)
{
}

static void mb_etv_term_stub(void)
{
}

static void mb_etv_timer_stub(void)
{
	*mb_lm_frclock() = *mb_lm_frclock() + 1u;
	if (*mb_lm_vblsem() == 0)
		*mb_lm_vbclock() = *mb_lm_vbclock() + 1u;
}

static void mb_common_init_lowmem(void)
{
	*mb_lm_etv_critic() = (uint32_t)(uintptr_t)mb_etv_critic_stub;
	*mb_lm_etv_term() = (uint32_t)(uintptr_t)mb_etv_term_stub;
	*mb_lm_etv_timer() = (uint32_t)(uintptr_t)mb_etv_timer_stub;
	*mb_lm_vbclock() = 0;
	*mb_lm_frclock() = 0;
	*mb_lm_hz_200() = 0;
	*mb_lm_vblsem() = 0;
	*mb_lm_v_bas_ad() = 0xffffffffu;
	mb_linea_init();
}

void mb_common_set_st_ram(uint32_t base, uint32_t size)
{
	uint32_t membot = base;
	uint32_t memtop = base + size;

	if (membot < 0x1000u)
		membot = 0x1000u;
	if (membot > memtop)
		membot = memtop;

	*mb_lm_phystop() = memtop;
	*mb_lm_membot() = membot;
	*mb_lm_memtop() = memtop;
	*mb_lm_memvalid() = 0x752019f3u;
	*mb_lm_memval2() = 0x237698aau;
	*mb_lm_memval3() = 0x5555aaaau;
	*mb_lm_longframe() = 1;
}

__attribute__((weak)) void mb_board_init_cookies(void)
{
}

__attribute__((weak)) uint32_t mb_board_kernel_tpa_start(void)
{
	return 0;
}

static void mb_common_init_boot_drive(void)
{
	uint32_t map = (uint32_t)mb_rom_dispatch.drvmap();
	uint16_t i;

	mb_boot_drive = 0xffffu;
	*mb_lm_drvbits() = map;
	if (map == 0)
		goto out;

	for (i = 0; i < 26; i++) {
		if (map & (1u << i)) {
			mb_boot_drive = i;
			break;
		}
	}
out:
	if (mb_boot_drive < 26)
		*mb_lm_bootdev() = mb_boot_drive;
	if (mb_boot_drive < 26)
		mb_bdos_set_current_drive(mb_boot_drive);
}

uint16_t mb_common_boot_drive(void)
{
	return mb_boot_drive;
}

void mb_common_setup_vectors(void)
{
	uint32_t *dst = (uint32_t *)(uintptr_t)0x8;
	uint32_t *src = mb_vector_table + 2;
	uint32_t i;

	if (src != dst) {
		for (i = 0; i < 254; i++)
			dst[i] = src[i];
	}

	mb_cpu_set_vbr(0);
}

uint32_t mb_common_vector_base(void)
{
	return 0;
}

void mb_common_run_tests(void);

void mb_common_start(void)
{
	mb_board_early_init();
	mb_cmdline[0] = '\0';
	mb_common_init_lowmem();
	mb_cookie_init_defaults();
	mb_board_init_cookies();
	mb_common_init_boot_drive();
	mb_common_setup_vectors();
	mb_cpu_enable_interrupts();
	mb_cpu_write_sr((uint16_t)(mb_cpu_read_sr() & (uint16_t)~0x2000u));
	mb_common_run_tests();
	{
		char kernel_path[384];
		if (mb_common_find_kernel_path(kernel_path, sizeof(kernel_path)) == 0) {
			mb_log_printf("mintboot: kernel candidate %s\n", kernel_path);
			if (mb_common_load_kernel(kernel_path, 1) != 0)
				mb_log_puts("mintboot: kernel load failed\n");
			else
				mb_log_puts("mintboot: kernel loaded (jump)\n");
		} else {
			mb_log_puts("mintboot: kernel not found\n");
		}
	}
	mb_log_puts("mintboot: common init complete\n");
	mb_board_exit(0);
	/* TODO: load/relocate kernel, finalize boot info, jump to entry. */
}
