#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_tests.h"
#include "mintboot/mb_lowmem.h"
#include "mb_tests_internal.h"

static void mb_tests_run_common(void)
{
	mb_fat_run_tests();
	mb_tests_setexc();
	mb_tests_gettime();
	mb_tests_drive_range();
	mb_tests_drive_path();
	mb_tests_bconmap();
	mb_tests_bios_bdos();
	mb_tests_vbclock();
}

static void mb_tests_reset_state(void)
{
	uint16_t drive = mb_portable_boot_drive();

	Dsetdrv((int16_t)drive);
	if (Dsetpath("\\") != 0)
		mb_panic("Test reset: Dsetpath failed");
}

void mb_portable_run_tests(void)
{
	uint32_t memtop = *mb_lm_memtop();
	uint32_t phystop = *mb_lm_phystop();
	uint32_t user_sp = memtop;
	uint32_t super_sp;
	uint32_t saved_sp;
	long prev_super_sp;

	mb_tests_run_common();
	mb_tests_reset_state();
	/*
	 * Keep user-mode stack away from the supervisor stack so trap entry
	 * frames cannot corrupt local test state.
	 */
	if (user_sp > 0x10000u)
		user_sp -= 0x10000u;
	user_sp &= ~3u;
	super_sp = (phystop > 0x1000u) ? ((phystop - 0x1000u) & ~3u) : (phystop & ~3u);
	mb_log_printf("tests: memtop=%08x phystop=%08x user_sp=%08x super_sp=%08x\r\n",
		      memtop, phystop, user_sp, super_sp);
	__asm__ volatile("move.l %%sp, %0" : "=d"(saved_sp));
	__asm__ volatile("move.l %0, %%usp" : : "a"(user_sp));
	(void)Super((void *)(uintptr_t)super_sp);
	mb_tests_run_common();
	prev_super_sp = Super(0);
	(void)prev_super_sp;
	__asm__ volatile("move.l %0, %%sp" : : "d"(saved_sp));
	mb_tests_kernel_loader();
	mb_tests_reset_state();
	mb_coverage_dump();
}
