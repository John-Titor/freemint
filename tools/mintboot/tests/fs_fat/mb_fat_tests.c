#include "mintboot/mb_tests.h"
#include "mb_fat_tests_internal.h"

void mb_fat_run_tests(void)
{
	struct mb_fat_test_ctx t;
	const struct mb_test_bpb *bpb;

	mb_tests_init_drive();
	mb_tests_snapshot_fs(mb_tests_drive_dev);
	mb_fat_tests_init_context(&t);

	mb_log_puts("mintboot: FAT test start\r\n");
	bpb = mb_tests_getbpb();
	if (bpb->clsiz <= 0 || bpb->rdlen <= 0 || bpb->datrec <= 0 ||
	    bpb->numcl == 0)
		mb_panic("FAT test: BPB fields invalid");

	mb_fat_tests_phase_core_helpers(&t);
	mb_fat_tests_phase_lookup_io(&t);
	mb_fat_tests_phase_rename_attr_seek(&t);
	mb_fat_tests_phase_dir_integrity(&t);

	mb_log_puts("mintboot: FAT test done\r\n");
	mb_tests_restore_fs();
}
