#include "mb_fat_tests_internal.h"

void mb_fat_tests_phase_dir_integrity(struct mb_fat_test_ctx *t)
{
	long rc;

	rc = mb_fat_check(mb_tests_drive_dev, &t->report);
	if (rc != 0)
		mb_panic("FAT test: integrity rc=%d bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 (int)rc, t->report.bad_bpb, t->report.bad_dirent,
			 t->report.bad_chain, t->report.bad_fat, t->report.lost_clusters,
			 t->report.cross_links);
	if (t->report.bad_bpb || t->report.bad_dirent || t->report.bad_chain ||
	    t->report.bad_fat || t->report.lost_clusters || t->report.cross_links)
		mb_panic("FAT test: integrity bad_bpb=%u bad_dirent=%u bad_chain=%u bad_fat=%u lost=%u cross=%u",
			 t->report.bad_bpb, t->report.bad_dirent, t->report.bad_chain,
			 t->report.bad_fat, t->report.lost_clusters, t->report.cross_links);
}
