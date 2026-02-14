#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mintboot/mb_tests.h"
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
	mb_tests_run_common();
	mb_tests_reset_state();
	mb_tests_kernel_loader();
	mb_tests_reset_state();
	mb_coverage_dump();
}
